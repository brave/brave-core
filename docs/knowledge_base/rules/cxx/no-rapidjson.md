---
name: brave/no-rapidjson
description:
  'JSON is parsed and written with base::JSONReader and base::JSONWriter, never
  rapidjson.'
rubric: design
severity: should_fix
---

**Rule.** NEVER add a use of, or a dependency on, rapidjson. Parse JSON with
`base::JSONReader` and serialise it with `base::JSONWriter`.

**Why.** The base libraries are Chromium's standard, already linked into
everything, and produce `base::Value`s the rest of the code works with.

## Examples

Bad:

```cpp
rapidjson::Document document;
document.Parse(json.c_str());
```

Good:

```cpp
std::optional<base::Value::Dict> dict = base::JSONReader::ReadDict(json);
```
