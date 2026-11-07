---
name: overlap/consolidated-entry-points
rubric: overlap
severity: regression
---

**Rule.** When upstream merges several entry points into a new one, the new one
still goes through Brave's `_ChromiumImpl` wrapper or override.

**Why.** The new entry point is called directly, so Brave's restriction on the
old ones silently stops applying.

**How to check.** For each embedder virtual Brave overrides that a candidate
touches, compare upstream's callers at both tags. When a candidate moves a
function out of a file Brave replaces whole into a shared one, grep `changes`
for the new location's name: its new callers bypass the replacement.

## Examples

- `AllowServiceWorker()` and friends became `IsAnyStorageAccessAllowed()`, which
  bypassed Request-OTR.
- M159, 8145.1: `chrome::GetDesktopName()` moved to
  `version_info::nix::GetDesktopName()`, which the Camera portal calls directly,
  past Brave's `channel_info_posix.cc` replacement.
