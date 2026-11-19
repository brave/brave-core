---
name: brave/fixed-flat-map-for-constant-lookup-tables
description:
  'Compile-time constant lookup tables, such as enum-to-string mappings, are
  base::MakeFixedFlatMap/base::MakeFixedFlatSet constexpr tables, not
  runtime-built std::map/base::flat_map constants or switch statements.'
rubric: perf
severity: should_fix
---

**Rule.** When a mapping or set is fully known at compile time (enum to string,
string to enum, a list of allowed hosts or keys), declare it as
`constexpr auto kTable = base::MakeFixedFlatMap<K, V>({...});` or
`base::MakeFixedFlatSet<T>({...})`, with `std::string_view` for strings. Do not
build it at runtime as a `const std::map`, a `base::flat_map` or a
`base::NoDestructor` container, and prefer it to a long `switch` that only maps
cases to constants.

**Why.** A fixed flat map is built and sorted at compile time, so it costs no
static initialiser or allocation, and duplicate keys fail the build. It keeps
the whole mapping in one table that is easier to extend than a switch.

## Examples

Bad:

```cpp
const std::map<ActionType, std::string> kActionNames = {
    {ActionType::kSummarize, "summarize"},
    {ActionType::kRewrite, "rewrite"},
};
```

Good:

```cpp
constexpr auto kActionNames =
    base::MakeFixedFlatMap<ActionType, std::string_view>({
        {ActionType::kSummarize, "summarize"},
        {ActionType::kRewrite, "rewrite"},
    });
```
