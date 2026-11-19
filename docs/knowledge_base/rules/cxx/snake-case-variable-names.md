---
name: brave/snake-case-variable-names
description:
  'C++ variables, parameters and members use snake_case, never camelCase, which
  is kept for class and method names.'
rubric: readability
severity: should_fix
---

**Rule.** Name C++ variables, function parameters and data members in snake*case
(`user_name`, `tab_count`,
`user_name*` for a class member). NEVER name a variable in camelCase (`userName`);
capitalised camel case is for class and method names only.

**Why.** Chromium's style guide fixes these conventions, so a camelCase variable
stands out in Brave code and reads like a type or a function.

## Examples

Bad:

```cpp
bool isTorEnabled = false;
std::string userName;
```

Good:

```cpp
bool is_tor_enabled = false;
std::string user_name;
```
