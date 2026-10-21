# Known Issues

## Open Issues

- **Allocation failure coverage:** Deterministic `malloc()`/`realloc()` failure injection is not implemented, so recovery at every allocation site is not exhaustively tested.
- **Terminal output failure coverage:** The integration suite verifies normal terminal cleanup and signal paths, but does not inject failures into terminal `write()` calls or verify every partial-output failure.
- **Unicode display width:** Wide code points and combining marks use POSIX `wcwidth()` plus fallback ranges, but this is not a complete, versioned Unicode width database. Emoji ZWJ grapheme clusters, variation-selector presentation, and terminal-specific width disagreements remain possible.
- **Filename display:** Non-ASCII filename bytes are preserved for filesystem operations but currently displayed as `?` in the Status Bar and filename-entry prompt.
- **Configuration:** Built-in tab width defaults to four columns; loading user configuration files and applying command-line configuration overrides are not implemented.
- **Rendering strategy:** Incremental or dirty-cell rendering is not implemented; the Renderer uses full-frame redraws.
- **Save behavior:** There is no separate Save As command for a document that already has an associated path; Ctrl-S saves to its current path. For unnamed documents, Ctrl-S activates filename entry.

Chapter 54.7 verification: Debug with diagnostics, sanitizer-enabled, warning-clean, and Release builds passed all ten CTest targets. Automated pseudo-terminal tests verify clean quit, resize handling, SIGINT/SIGTERM/SIGHUP cleanup, terminal attribute restoration, alternate-screen and wrap-mode restoration, and startup-load failure cleanup. Allocation and terminal-write failure injection remain open.

## Notes

This file records unresolved implementation problems discovered during development. Resolved issues should not remain here; significant historical information belongs in Git history or implementation notes.




## Logging Notes

- The log path is currently fixed to `log.txt` relative to the process working directory; command-line or user-configuration log-path selection is not implemented.
- Logging is buffered and best-effort. A filesystem failure may cause the tail of a session to be lost; it must not interrupt editing.
