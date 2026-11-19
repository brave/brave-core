---
name: brave/json-value-converter-over-manual-parsing
description:
  'When JSON is parsed into a C++ type, prefer base::JSONValueConverter over
  reading the dictionary key by key.'
rubric: readability
severity: should_fix
---

**Rule.** When a change converts a parsed JSON dictionary into a C++ struct or
class, it registers the fields with a `base::JSONValueConverter<T>`
(`RegisterJSONConverter()` with `RegisterStringField()`, `RegisterIntField()`
and the like) instead of a sequence of `Find*()` calls that copy each key into
the result by hand.

**Why.** Key-by-key parsing repeats the same lookup-check-assign boilerplate for
every field, which is verbose and easy to get wrong (a missed check, a typo in a
key, a field forgotten). The converter declares the mapping once and applies it
uniformly.

## Examples

Bad:

```cpp
const auto* name = dict->FindString("name");
const auto age = dict->FindInt("age");
if (name) result.name = *name;
if (age) result.age = *age;
```

Good:

```cpp
static void RegisterJSONConverter(
    base::JSONValueConverter<MyType>* converter) {
  converter->RegisterStringField("name", &MyType::name);
  converter->RegisterIntField("age", &MyType::age);
}
```
