---
name: brave/ui-text-use-contractions
description:
  User-facing strings use contractions (can't, isn't, you're) where possible
  instead of the spelled-out forms.
rubric: readability
severity: nit
---

**Rule.** Use contractions in UI text where possible: "can't" for "cannot",
"isn't" for "is not", "you're" for "you are", "don't" for "do not".

**Why.** Contractions are more conversational, matching Brave's voice, and they
save space in UI strings.

## Examples

Bad:

```xml
<message name="IDS_DELETE_WARNING" desc="Warning in the delete confirmation dialog">
  You cannot undo this action.
</message>
```

Good:

```xml
<message name="IDS_DELETE_WARNING" desc="Warning in the delete confirmation dialog">
  You can't undo this action.
</message>
```
