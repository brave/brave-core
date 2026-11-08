---
name: brave/filename-matches-class-name
description:
  'A class lives in files named after it, such as webcompat_reporter_service.h
  for WebcompatReporterService.'
rubric: readability
severity: should_fix
---

**Rule.** Name the header and source files of a class after the class, in
snake_case: `WebcompatReporterService` is declared in
`webcompat_reporter_service.h` and defined in `webcompat_reporter_service.cc`.
When a change renames a class, it renames its files too.

**Why.** Readers and tools find a class from its name. A file named after
something else, or after the class's old name, sends them looking in the wrong
place.

## Examples

Bad:

```cpp
// brave/components/webcompat_reporter/browser/reporter.h
class WebcompatReporterService { ... };
```

Good:

```cpp
// brave/components/webcompat_reporter/browser/webcompat_reporter_service.h
class WebcompatReporterService { ... };
```
