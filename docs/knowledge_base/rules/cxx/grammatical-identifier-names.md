---
name: brave/grammatical-identifier-names
description:
  'Names of functions, classes and variables are grammatically correct English,
  such as IsBraveCommandId() for one id.'
rubric: readability
severity: nit
---

**Rule.** Make every new name read as correct English: singular for one thing,
plural for a collection, and verb forms that agree, as in
`IsBraveCommandId(int id)` for a check of a single id.

**Why.** A name that is grammatically wrong misdescribes what it holds or does:
`IsBraveCommandIds(int id)` suggests a check of several ids.

## Examples

Bad:

```cpp
bool IsBraveCommandIds(int id);
```

Good:

```cpp
bool IsBraveCommandId(int id);
```
