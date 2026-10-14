---
name: obsolete/replaced-headers-keep-deleted-names
rubric: obsolete
severity: obsolete
---

**Rule.** A `chromium_src` header that replaces the original outright (never
includes it) doesn't declare what upstream deleted.

**Why.** A replacement copies upstream's declarations, and keeps them after
upstream deletes them: dead constants and functions nothing compiles against.

**How to check.** The candidates are the `header` rows of
`vpython3 tools/cr/codehealth/scripts/obsolete.py removed-names`: for each
replacing header, the constant and function names the original lacks at the new
tag, split into `deleted in range` and `not upstream` (deleted before the range,
or Brave's own: counted, listed with `--debt`).

## Examples

- M158, 8127.2: 26 dead constants in `chrome/common/url_constants.h`.
