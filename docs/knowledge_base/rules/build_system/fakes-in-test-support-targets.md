---
name: brave/fakes-in-test-support-targets
description:
  'Fake and mock service implementations go in a test_support target that test
  targets depend on, not directly in a unit_tests target.'
rubric: testing
severity: should_fix
---

**Rule.** Put the sources of a fake or mock service in a `testonly`
`test_support` target (the component's existing one, if it has one), and make
the `unit_tests` target depend on it. Do not list the fake's files in the
`sources` of a `unit_tests` target.

**Why.** A fake compiled into one `unit_tests` target can only be used by that
target. In `test_support`, test targets across the codebase reuse the same fake
instead of writing their own.

## Examples

Bad:

```gn
source_set("unit_tests") {
  sources = [
    "my_service_unittest.cc",
    "test/fake_dependency_service.cc",
    "test/fake_dependency_service.h",
  ]
}
```

Good:

```gn
source_set("test_support") {
  testonly = true
  sources = [
    "test/fake_dependency_service.cc",
    "test/fake_dependency_service.h",
  ]
  deps = [ ":dependency_service" ]
}

source_set("unit_tests") {
  sources = [ "my_service_unittest.cc" ]
  deps = [ ":test_support" ]
}
```
