# Known Issues

## Open Issues

- **Terminal resize:** Terminal resize events and Screen Buffer/Viewport reinitialization are not implemented.
- **Allocation failure coverage:** Allocation-failure paths and a broader terminal-size integration matrix need additional coverage.
- **Unicode display width:** Wide code points and combining marks use POSIX `wcwidth()` plus fallback ranges, but this is not a complete, versioned Unicode width database. Emoji ZWJ grapheme clusters, variation-selector presentation, and terminal-specific width disagreements remain possible.
- **Filename display:** Non-ASCII filename bytes are preserved for filesystem operations but currently displayed as `?` in the Status Bar and filename-entry prompt.
- **Configuration:** Built-in tab width defaults to four columns; loading user configuration files and applying command-line configuration overrides are not implemented.
- **Rendering strategy:** Incremental or dirty-cell rendering is not implemented; the Renderer uses full-frame redraws.
- **Save behavior:** There is no separate Save As command for a document that already has an associated path; Ctrl-S saves to its current path. For unnamed documents, Ctrl-S activates filename entry.

Chapter 53.11 verification: fresh Debug and warning-clean builds passed all eight CTest targets. A pseudo-terminal editing/save/exit run verified the expected saved bytes `bc\t\n`, exit status zero, and alternate-screen restoration. No new unresolved defect was identified in this milestone.

## Notes

This file records unresolved implementation problems discovered during development. Resolved issues should not remain here; significant historical information belongs in Git history or implementation notes.

