---
name: brave/no-trailing-underscore-on-struct-members
description:
  'Data members of plain structs are named without a trailing underscore; the
  trailing underscore is for class data members.'
rubric: readability
severity: nit
---

**Rule.** Name the fields of a plain `struct` (a passive aggregate of data) like
ordinary variables: `name`, `percentage`. Reserve the trailing underscore
(`name_`) for data members of classes.

**Why.** This is the Google C++ naming convention Chromium follows. The
underscore marks private class state that is accessed through methods; struct
fields are accessed directly by their users (`site.name`), where an underscore
would only add noise.

## Examples

Bad:

```cpp
struct ContentSite {
  std::string name_;
  int percentage_;
};
```

Good:

```cpp
struct ContentSite {
  std::string name;
  int percentage = 0;
};
```
