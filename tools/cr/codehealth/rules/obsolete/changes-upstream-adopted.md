---
name: obsolete/changes-upstream-adopted
rubric: obsolete
severity: obsolete
---

**Rule.** A Brave patch line that upstream now has itself is dropped from the
patch.

**Why.** The patch then repeats upstream's line, or keeps a hunk only to re-sort
lines upstream added, and the duplicate can change behaviour.

**How to check.** The candidates are
`vpython3 tools/cr/codehealth/scripts/obsolete.py patch-adoptions`: per patch,
the `+` lines whose count in the pristine file grew across the range
(`adopted`), and the patched files whose every non-generic `+` line the pristine
file has at both tags, unless the patch adds a line naming Brave
(`already upstream`, which predates the range: counted, listed with `--debt`).
It counts occurrences rather than testing membership, since the line can already
exist elsewhere in the file. Line counts miss an adoption under another name:
read the overlap subjects for a Brave hook's behaviour.

## Examples

- The Linux EULA default.
- M157, 8103.1: `chrome-browser-notifications-BUILD.gn.patch` kept a hunk only
  to re-sort lines upstream added.
- M157, 8097.1: the `HTMLParserScriptRunner` guard, upstream since M146.
- M158, 8127.2: upstream's `UpdateSessionCountryAndGeoLevel1()` on 304 responses
  duplicates Brave's variations `SetSessionCountry` hook (another name: found by
  reading subjects).
