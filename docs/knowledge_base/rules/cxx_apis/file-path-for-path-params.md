---
name: brave/file-path-for-path-params
description:
  'Parameters that are file system paths are base::FilePath, not std::string.'
rubric: design
severity: should_fix
---

**Rule.** Declare a parameter that holds a path on disk (a profile directory, a
file to read or write, an install location) as `const base::FilePath&`. Do not
pass paths around as `std::string`, `std::string_view` or `std::u16string`;
convert at the boundary that really needs a string (logging, IPC fields that are
strings, UI display).

**Why.** `base::FilePath` carries the platform's native encoding and separator,
offers the path operations (`Append()`, `DirName()`, `Extension()`), and tells
the reader what the parameter is. A string loses the encoding on Windows and
invites ad hoc concatenation of paths.

## Examples

Bad:

```cpp
std::string GetProfileId(const std::string& profile_path);
```

Good:

```cpp
std::string GetProfileId(const base::FilePath& profile_path);
```
