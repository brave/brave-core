---
name: brave/use-mojo-generated-constants
description:
  'Enums and constants defined in Mojo are used through their generated C++,
  Java and JS bindings, never redefined by hand.'
rubric: correctness
severity: should_fix
---

**Rule.** When an enum or constant is defined in a `.mojom` file, every language
uses the binding generated from it (`mojom::Foo` in C++, the generated Java and
TS/JS types). Do not define a matching enum, `@IntDef` or constant by hand in
another language.

**Why.** Hand-written copies drift from the Mojo definition: a value added or
renumbered there is silently wrong in the copy.

## Examples

Bad:

```ts
// Mirrors mojom::ContentType.
export const enum ContentType {
  kText = 0,
  kImage = 1,
}
```

Good:

```ts
import { ContentType } from 'gen/brave/components/foo/common/foo.mojom.m.js'
```
