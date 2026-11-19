---
name: brave/gn-target-name-matches-directory
description:
  'A GN source_set is named after its directory, so that it is referenced by the
  directory path alone.'
rubric: readability
severity: nit
---

**Rule.** Name a directory's main `source_set` after the directory, so that
`//brave/components/brave_referrals/browser` refers to it. Do not give it an
unrelated name that every reference has to spell out.

**Why.** A target named after its directory is found from its path alone, which
keeps `deps` predictable and short.

## Examples

Bad:

```gn
# In brave/components/brave_referrals/browser/BUILD.gn
source_set("referrals") { ... }
# Referenced as //brave/components/brave_referrals/browser:referrals
```

Good:

```gn
# In brave/components/brave_referrals/browser/BUILD.gn
source_set("browser") { ... }
# Referenced as //brave/components/brave_referrals/browser
```
