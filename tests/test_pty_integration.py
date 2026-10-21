#!/usr/bin/env python3
"""Pseudo-terminal integration checks for editor startup and cleanup."""

import fcntl
import os
import pty
import select
import signal
import struct
import subprocess
import termios
import tempfile
import time


def set_window_size(fd, rows, columns):
    """Set a pseudo-terminal's dimensions."""
    fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", rows, columns, 0, 0))


def read_available(fd, timeout=0.15):
    """Read currently available pseudo-terminal output without blocking."""
    output = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        ready, _, _ = select.select([fd], [], [], max(0, remaining))
        if not ready:
            break
        try:
            chunk = os.read(fd, 65536)
        except OSError:
            break
        if not chunk:
            break
        output.extend(chunk)
    return bytes(output)


def launch_editor(executable, rows=24, columns=80, arguments=(), cwd=None):
    """Launch the editor attached to a configured pseudo-terminal."""
    master_fd, slave_fd = pty.openpty()
    set_window_size(slave_fd, rows, columns)
    original_attributes = termios.tcgetattr(slave_fd)
    process = subprocess.Popen(
        [executable, *arguments],
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        close_fds=True,
        cwd=cwd,
    )
    return process, master_fd, slave_fd, original_attributes


def finish_and_check(
    process, master_fd, slave_fd, original_attributes, expected_status=0, prior_output=b""
):
    """Wait for editor cleanup and verify its terminal restoration output."""
    try:
        process.wait(timeout=3)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=3)
        raise AssertionError("editor did not terminate promptly")

    output = prior_output + read_available(master_fd, 0.1)
    restored_attributes = termios.tcgetattr(slave_fd)

    assert process.returncode == expected_status, process.returncode
    assert b"\x1b[?1049l" in output, "alternate screen was not restored"
    assert b"\x1b[?7h" in output, "terminal wrap mode was not restored"
    assert restored_attributes == original_attributes, "terminal attributes were not restored"
    os.close(slave_fd)
    os.close(master_fd)
    return output


def wait_for_output(master_fd, marker, timeout=3):
    """Wait until the editor has emitted a particular terminal sequence."""
    output = bytearray()
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        output.extend(read_available(master_fd, 0.1))
        if marker in output:
            return bytes(output)
    raise AssertionError(f"did not observe terminal sequence {marker!r}")


def test_clean_quit(executable):
    """Check startup, Ctrl-Q shutdown, and alternate-screen cleanup."""
    process, master_fd, slave_fd, original_attributes = launch_editor(executable)
    initial_output = wait_for_output(master_fd, b"\x1b[?1049h")
    os.write(master_fd, b"\x11")  # Ctrl-Q
    finish_and_check(
        process, master_fd, slave_fd, original_attributes, prior_output=initial_output
    )


def test_input_and_dirty_flag_logging(executable, expect_logging):
    """Check that recognized input and Dirty Flag transitions reach log.txt."""
    with tempfile.TemporaryDirectory(prefix="tedit-log-pty-") as directory:
        process, master_fd, slave_fd, original_attributes = launch_editor(
            executable, cwd=directory
        )
        initial_output = wait_for_output(master_fd, b"\x1b[?1049h")
        os.write(master_fd, b"x")
        time.sleep(0.1)
        os.write(master_fd, b"\x11")  # Ctrl-Q prompts because the document is dirty.
        prompt_output = wait_for_output(master_fd, b"Save changes before quitting?")
        os.write(master_fd, b"n")  # Discard the test edit and exit.
        finish_and_check(
            process, master_fd, slave_fd, original_attributes,
            prior_output=initial_output + prompt_output,
        )

        log_path = os.path.join(directory, "log.txt")
        if expect_logging:
            with open(log_path, "r", encoding="utf-8") as stream:
                log_contents = stream.read()
            assert "[DEBUG] [input] recognized event=character codepoint=U+0078" in log_contents
            assert "Dirty Flag changed: clean -> dirty" in log_contents
            assert "[DEBUG] [input] recognized event=ctrl-q" in log_contents
            assert "document contents" not in log_contents.lower()
        else:
            assert not os.path.exists(log_path), "logging-disabled build created log.txt"


def test_resize_and_quit(executable):
    """Check that SIGWINCH is processed and the editor remains usable."""
    process, master_fd, slave_fd, original_attributes = launch_editor(executable)
    initial_output = wait_for_output(master_fd, b"\x1b[?1049h")
    set_window_size(master_fd, 30, 100)
    os.kill(process.pid, signal.SIGWINCH)
    # Give the event loop time to query dimensions and render the new frame.
    time.sleep(0.15)
    if process.poll() is not None:
        raise AssertionError("editor exited while handling a valid resize")
    os.write(master_fd, b"\x11")
    finish_and_check(
        process, master_fd, slave_fd, original_attributes, prior_output=initial_output
    )


def test_signal_cleanup(executable, termination_signal):
    """Check cleanup after an externally delivered termination signal."""
    process, master_fd, slave_fd, original_attributes = launch_editor(executable)
    initial_output = wait_for_output(master_fd, b"\x1b[?1049h")
    os.kill(process.pid, termination_signal)
    finish_and_check(
        process, master_fd, slave_fd, original_attributes, prior_output=initial_output
    )


def test_startup_load_failure(executable):
    """Check cleanup when loading the requested startup file fails."""
    with tempfile.TemporaryDirectory(prefix="tedit-pty-") as directory:
        missing_path = os.path.join(directory, "does-not-exist.txt")
        process, master_fd, slave_fd, original_attributes = launch_editor(
            executable, arguments=(missing_path,)
        )
        initial_output = wait_for_output(master_fd, b"\x1b[?1049h")
        output = finish_and_check(
            process, master_fd, slave_fd, original_attributes, expected_status=1,
            prior_output=initial_output
        )
        assert b"unable to open or decode" in output, output


def main():
    import sys

    executable = os.path.abspath(sys.argv[1])
    expect_logging = len(sys.argv) > 2 and sys.argv[2] == "--expect-logging"
    test_clean_quit(executable)
    test_input_and_dirty_flag_logging(executable, expect_logging)
    test_resize_and_quit(executable)
    test_signal_cleanup(executable, signal.SIGINT)
    test_signal_cleanup(executable, signal.SIGTERM)
    test_signal_cleanup(executable, signal.SIGHUP)
    test_startup_load_failure(executable)
    print("pseudo-terminal integration checks passed")


if __name__ == "__main__":
    main()
