---
name: brave/auto-only-for-verbose-types
description:
  'auto is not used to avoid writing a short, descriptive type such as
  base::TimeDelta; any comment on it is a nit the author may decline.'
rubric: readability
severity: nit
---

**Rule.** Spell out a type that is short and tells the reader something, such as
`base::TimeDelta` or `base::Time`, instead of declaring the variable `auto`.
Keep `auto` where the type is long or nested and adds nothing, such as
`std::optional<std::vector<uint8_t>>`. This is a preference: always prefix the
comment with `nit:`, and NEVER insist when the author declines.

**Why.** The Google style guide says not to use `auto` merely to avoid the
inconvenience of writing a type: a short explicit type documents the value at
the point it is declared.

## Examples

Bad:

```cpp
auto elapsed = timer.Elapsed();
```

Good:

```cpp
base::TimeDelta elapsed = timer.Elapsed();

// The type is std::optional<std::vector<uint8_t>>.
auto weights = base::ReadFileToBytes(weights_path);
```
