---
name: brave/no-instanceof-on-mojom-types
description:
  Do not use instanceof on types from TypeScript WebUI mojom bindings, which are
  interfaces rather than classes; use a type guard or a discriminated union.
rubric: correctness
severity: should_fix
---

**Rule.** Do not check a value against a mojom type with `instanceof`: the
TypeScript WebUI mojom bindings generate interfaces, not classes, for mojom
structs and unions. Tell the types apart with a type guard, a property check, or
a discriminated union.

**Why.** An interface exists only at compile time. There is no constructor to
compare the value against, so the check cannot do what it says.

## Examples

Bad:

```tsx
if (value instanceof mojom.ConversationTurn) { ... }
```

Good:

```tsx
if ('text' in value && 'role' in value) { ... }
```
