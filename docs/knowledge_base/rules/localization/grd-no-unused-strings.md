---
name: brave/grd-no-unused-strings
description:
  GRD/GRDP strings are added only when the code uses them, and removed when a
  change leaves them unused.
rubric: correctness
severity: should_fix
---

**Rule.** Add a `<message>` only when the code references its `IDS_` constant,
and check that it does. When a change removes the last use of a string, remove
the string from its GRD/GRDP file too.

**Why.** Every string is sent to translators and translated into every supported
language, which costs time and money even if no one ever sees it.

## Examples

Bad:

```xml
<message name="IDS_FOO_LEGACY_TITLE" desc="Title of the old Foo dialog, no longer shown">
  Foo settings
</message>
```

Good:

```xml
<!-- IDS_FOO_LEGACY_TITLE removed together with its last use. -->
```
