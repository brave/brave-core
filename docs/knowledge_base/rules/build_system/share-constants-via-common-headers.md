---
name: brave/share-constants-via-common-headers
description:
  'A constant used by several files (implementation, tests, other classes) is
  defined once in a common header, not redefined in each.'
rubric: design
severity: should_fix
---

**Rule.** Define a constant that more than one file needs (a limit, a URL, a
key) once, as an `inline constexpr` in a common header of the component that
owns it, and include that header where it is used. Do not repeat the same
constant across implementation, test and other files.

**Why.** Duplicated definitions drift: one copy changes and the others keep the
old value, so tests pass against a value production no longer uses.

## Examples

Bad:

```cpp
// model_service.cc
constexpr char kOllamaEndpoint[] = "http://localhost:11434";
// model_service_unittest.cc
constexpr char kOllamaEndpoint[] = "http://localhost:11434";
```

Good:

```cpp
// common/constants.h
inline constexpr char kOllamaEndpoint[] = "http://localhost:11434";
```
