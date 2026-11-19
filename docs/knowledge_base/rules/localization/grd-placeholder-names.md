---
name: brave/grd-placeholder-names
description:
  GRD/GRDP placeholders have descriptive SCREAMING_SNAKE_CASE names, with
  LINK_BEGIN/LINK_END for links.
rubric: readability
severity: should_fix
---

**Rule.** Give every `<ph>` a descriptive `name` in SCREAMING_SNAKE_CASE that
says what it stands for (`SITE_NAME`, `TAB_COUNT`), not a generic one such as
`PH_1`. Name link placeholders `LINK_BEGIN` and `LINK_END`.

**Why.** Translators see the placeholder name; a descriptive one tells them what
the sentence is about and how to inflect around it.

## Examples

Bad:

```xml
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation dialog for shredding site data">
  Shredding will delete site data for <ph name="PH_1">$1<ex>example.com</ex></ph>.
</message>
```

Good:

```xml
<message name="IDS_SHRED_CONFIRMATION" desc="Confirmation dialog for shredding site data">
  Shredding will delete site data for <ph name="SITE_NAME">$1<ex>example.com</ex></ph>.
</message>
```
