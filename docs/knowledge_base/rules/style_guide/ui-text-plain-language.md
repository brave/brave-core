---
name: brave/ui-text-plain-language
description:
  General product UI text aims for a fifth-grade reading level and avoids
  jargon, defining any technical term at its first use.
rubric: readability
severity: should_fix
---

**Rule.** Keep general product UI text at about a fifth-grade reading level:
short, common words and simple sentences. Do not use jargon unless the surface
targets a technical audience, and define any term the reader may not know where
it first appears.

**Why.** Most Brave users are not technical. Text they cannot read at a glance
is skipped or misunderstood, and jargon is also harder to translate.

## Examples

Bad:

```xml
<message name="IDS_PAGE_LOAD_ERROR" desc="Error shown when a page fails to load">
  The page could not be loaded due to a network connectivity issue.
</message>
```

Good:

```xml
<message name="IDS_PAGE_LOAD_ERROR" desc="Error shown when a page fails to load">
  This page failed to load. Check your connection and try again.
</message>
```
