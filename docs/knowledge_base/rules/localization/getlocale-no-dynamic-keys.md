---
name: brave/getlocale-no-dynamic-keys
description:
  getLocale() is never called with a dynamically constructed key; each call
  names its string explicitly.
rubric: correctness
severity: blocking
---

**Rule.** NEVER build the key passed to `getLocale()` at runtime (template
literals, concatenation, case conversion, lookups by a variable). Pass an
explicit string constant (`S.CHAT_UI_CLAUDE_SUBTITLE`), or have the data source
(server API, mojom struct) provide the string itself.

**Why.** Dynamic keys make static analysis impossible, hide which strings are
used so dead ones cannot be found, and break silently when key formats change.

## Examples

Bad:

```tsx
getLocale(`CHAT_UI_${model.key.toUpperCase().replaceAll('-', '_')}_SUBTITLE`)
```

Good:

```tsx
getLocale(S.CHAT_UI_CLAUDE_SUBTITLE)
```
