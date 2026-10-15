# Known Issues

## Open Issues

Non-ASCII filename display remains limited: UTF-8 path bytes are preserved for filesystem operations, but the Status Bar and filename-entry prompt currently render non-ASCII bytes as `?`. Document Delete, Home, End, Page Up, Page Down, and Tab remain deferred to the earlier navigation/editing milestones; Delete is currently supported only in filename entry.

## Notes

This file records unresolved implementation problems discovered during development.

Resolved issues should not remain here; significant historical information belongs in Git history or implementation notes.
