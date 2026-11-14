---
name: brave/ios-test-targets-in-brave-all-unit-tests
description:
  'Every new iOS test target is reachable from brave_all_unit_tests, or it never
  runs on CI.'
rubric: testing
severity: blocking
---

**Rule.** A new iOS test target MUST be added to the deps of a target that
`brave_all_unit_tests` runs (on iOS, `//brave/ios/testing:ios_brave_unit_tests`
and the targets listed in `brave_all_unit_tests_deps`).

**Why.** CI runs `brave_all_unit_tests`; a test target outside it builds nowhere
and its tests never run, so failures go unnoticed.

## Examples

Bad:

```gn
# ios/browser/playlist/BUILD.gn
source_set("unit_tests") {
  testonly = true
  sources = [ "playlist_service_unittest.mm" ]
}
# ...and nothing depends on it.
```

Good:

```gn
# ios/testing/BUILD.gn
test("ios_brave_unit_tests") {
  deps = [
    "//brave/ios/browser/playlist:unit_tests",
  ]
}
```
