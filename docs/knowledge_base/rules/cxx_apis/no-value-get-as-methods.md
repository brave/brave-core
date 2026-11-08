---
name: brave/no-value-get-as-methods
description:
  'Do not use the deprecated GetAs*() methods of base::Value (GetAsString(),
  GetAsInteger(), ...); use the direct accessors such as GetString(), GetInt()
  and GetDouble().'
rubric: readability
severity: should_fix
---

**Rule.** Do not call `GetAsString()`, `GetAsInteger()`, `GetAsBoolean()`,
`GetAsDouble()`, `GetAsList()` or `GetAsDictionary()` on a `base::Value`. Read
the value with the direct accessors (`GetString()`, `GetInt()`, `GetBool()`,
`GetDouble()`, `GetList()`, `GetDict()`), or with the `GetIf*()` accessors when
the type is not known (see [brave/value-get-if-accessors-for-unchecked-types]).

**Why.** The `GetAs*()` out-parameter methods are deprecated `base::Value` API.
The direct accessors return the value itself, so there is no out-parameter to
declare and no success flag to forget.

## Examples

Bad:

```cpp
std::string str;
value->GetAsString(&str);
```

Good:

```cpp
const std::string& str = value->GetString();
```
