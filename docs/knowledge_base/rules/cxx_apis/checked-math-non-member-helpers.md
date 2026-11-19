---
name: brave/checked-math-non-member-helpers
description:
  "Write checked arithmetic with base::CheckAdd(), base::CheckSub(),
  base::CheckMul() and the other non-member helpers, not with CheckedNumeric's
  operator overloads."
rubric: safety
severity: should_fix
---

**Rule.** Express each checked operation with its non-member helper
(`base::CheckAdd()`, `base::CheckSub()`, `base::CheckMul()`, `base::CheckDiv()`,
`base::CheckMod()`, `base::CheckLsh()`, `base::CheckRsh()`). Do not build
checked expressions from `+`, `-`, `*` and the like on a `base::CheckedNumeric`,
nor initialise a `CheckedNumeric` from an expression evaluated with plain
operators.

**Why.** With operators, any term computed before it meets the `CheckedNumeric`
(`count * item_size` in the example) is ordinary, unchecked arithmetic, and
nothing in the code shows it. The helpers name every checked operation, so no
part of the computation happens outside them.

## Examples

Bad:

```cpp
// `count * item_size` overflows before the CheckedNumeric is constructed.
base::CheckedNumeric<size_t> checked_total = count * item_size;
```

Good:

```cpp
size_t total = 0;
if (!base::CheckMul(count, item_size).AssignIfValid(&total)) {
  return std::nullopt;
}
```
