---
name: brave/no-plural-wrapper-types
description:
  'Do not add a plural or container type whose only content is a collection of
  the singular type; use a std::vector (or array) of the singular type directly.'
rubric: design
severity: should_fix
---

**Rule.** Do not define a struct, class or mojom struct (such as
`MonthlyStatements`) that only wraps a `std::vector` or `array<>` of another
type (`MonthlyStatement`). Pass and return the collection of the singular type
itself. A wrapper is justified only when it carries more than the collection
(other fields, invariants, behaviour).

**Why.** A wrapper that holds nothing but a list adds a type to name, convert,
construct and unwrap at every call site without adding any meaning or safety.

## Examples

Bad:

```cpp
struct MonthlyStatements {
  std::vector<MonthlyStatement> statements;
};
MonthlyStatements GetMonthlyStatements();
```

Good:

```cpp
std::vector<MonthlyStatement> GetMonthlyStatements();
```
