---
name: brave/ui-text-no-all-caps
description: User-facing text must never be written or rendered in ALL CAPS.
rubric: readability
severity: should_fix
---

**Rule.** NEVER write user-facing text in ALL CAPS, and NEVER render it so with
`text-transform: uppercase`, `android:textAllCaps` or `.uppercased()`. Acronyms
(VPN, BAT, URL) stay in capitals.

**Why.** All caps reads as shouting, is harder to read, and has no equivalent in
many scripts that translations use.

## Examples

Bad:

```xml
<message name="IDS_WARNING_LABEL" desc="Label above a security warning">
  WARNING: DANGEROUS SITE
</message>
```

Good:

```xml
<message name="IDS_WARNING_LABEL" desc="Label above a security warning">
  Warning: dangerous site
</message>
```
