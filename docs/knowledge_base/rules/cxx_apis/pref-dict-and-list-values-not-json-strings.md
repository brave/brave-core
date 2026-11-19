---
name: brave/pref-dict-and-list-values-not-json-strings
description:
  'Store structured data in prefs as dictionary or list prefs with
  SetDict()/SetList(), never as a JSON string written with JSONWriter and
  SetString().'
rubric: design
severity: should_fix
---

**Rule.** When a pref holds structured data, register it as a dictionary or list
pref (`RegisterDictionaryPref()`, `RegisterListPref()`) and write it with
`SetDict()`/`SetList()` (or a `ScopedDictPrefUpdate`/`ScopedListPrefUpdate`). Do
not serialise the data with `base::JSONWriter::Write()` and store the result
with `SetString()`, nor parse a string pref with `JSONReader` to read it back.

**Why.** Prefs are already stored as JSON. A JSON string inside a pref is
serialised twice, cannot be updated in place, has to be parsed (and its parse
errors handled) on every read, and is opaque to `brave://prefs-internals` and to
pref migration code.

## Examples

Bad:

```cpp
std::string result;
base::JSONWriter::Write(root, &result);
prefs->SetString(prefs::kMyPref, result);
```

Good:

```cpp
prefs->SetDict(prefs::kMyPref, std::move(dict_value));
prefs->SetList(prefs::kMyListPref, std::move(list_value));
```
