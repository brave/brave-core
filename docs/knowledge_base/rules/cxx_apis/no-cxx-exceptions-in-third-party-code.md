---
name: brave/no-cxx-exceptions-in-third-party-code
description:
  'C++ exceptions are disallowed in Chromium: integrated third-party libraries
  must build with exception support disabled, and no code may throw or catch.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER enable C++ exceptions for a target (for example by removing
`//build/config/compiler:no_exceptions` or adding
`//build/config/compiler:exceptions` to its `configs`), and never add `throw`,
`try` or `catch` to C++ code. When a change integrates a third-party library,
check that it builds and works with exceptions disabled (most libraries have a
no-exceptions mode or report errors through return values).

**Why.** Chromium is built with `-fno-exceptions`. Code that relies on
exceptions either fails to build or, with exceptions forced on for one target,
calls `std::terminate()` and crashes the moment an exception crosses into code
built without unwinding support.

## Examples

Bad:

```gn
source_set("foo_lib") {
  configs -= [ "//build/config/compiler:no_exceptions" ]
  configs += [ "//build/config/compiler:exceptions" ]
}
```

Good:

```gn
source_set("foo_lib") {
  # Built in the library's no-exceptions mode.
  defines = [ "FOO_NO_EXCEPTIONS" ]
}
```
