---
name: brave/non-optional-arrays-in-mojom
description:
  'Mojom uses a non-optional array, empty for no items, rather than array<T>?,
  unless null means something different from empty.'
rubric: design
severity: nit
---

**Rule.** Declare mojom struct fields and method parameters holding lists as
`array<T>`, with an empty array for "no items". Use `array<T>?` only when null
means something distinct from empty, and say what in a comment.

**Why.** An optional array gives two ways to say "nothing", which every client
must handle and which readers cannot tell apart.

## Examples

Bad:

```mojom
struct SearchResult {
  array<string>? queries;  // Is null different from []?
};
```

Good:

```mojom
struct SearchResult {
  array<string> queries;  // Empty array = no queries
};
```
