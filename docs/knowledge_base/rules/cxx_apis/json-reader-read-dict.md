---
name: brave/json-reader-read-dict
description:
  'Parse a JSON string that must be a dictionary with
  base::JSONReader::ReadDict(), not base::JSONReader::Read() followed by
  is_dict()/GetIfDict().'
rubric: readability
severity: nit
---

**Rule.** When the JSON being parsed is expected to be an object, call
`base::JSONReader::ReadDict(json)`, which returns
`std::optional<base::Value::Dict>`, and check that result. Do not call
`base::JSONReader::Read()` and then test `is_dict()` or extract
`GetIfDict()`/`GetDict()` by hand.

**Why.** `ReadDict()` folds the type check into the parse, returning nothing for
malformed JSON and for JSON of another type alike, and hands back the dictionary
itself instead of a `base::Value` to unwrap.

## Examples

Bad:

```cpp
auto value = base::JSONReader::Read(json_str);
if (!value || !value->is_dict()) return;
auto& dict = value->GetDict();
```

Good:

```cpp
auto dict = base::JSONReader::ReadDict(json_str);
if (!dict) return;
```
