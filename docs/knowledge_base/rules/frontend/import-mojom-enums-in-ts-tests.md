---
name: brave/import-mojom-enums-in-ts-tests
description:
  TypeScript tests import enum types from the generated Mojo bindings or the
  source files, and never redefine or duplicate them.
rubric: testing
severity: should_fix
---

**Rule.** In a TypeScript test, import every enum the test uses from the
generated Mojo bindings (`gen/.../*.mojom-webui.js`) or from the source module
that defines it. Do NOT declare an enum, or an object of its values, that copies
one defined elsewhere.

**Why.** A copied enum does not change when the original does: the test keeps
passing against values the real code no longer uses, and stops testing what
ships.

## Examples

Bad:

```tsx
enum FileType {
  kImage = 0,
  kDocument = 1,
}
```

Good:

```tsx
import { FileType } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom-webui.js'
```
