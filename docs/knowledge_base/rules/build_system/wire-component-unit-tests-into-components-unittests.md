---
name: brave/wire-component-unit-tests-into-components-unittests
description:
  "A component's unit tests live in its own :unit_tests target, wired into
  brave_components_unittests unless they need chrome/browser-layer test support."
rubric: testing
severity: should_fix
---

**Rule.** List unit test files in a `unit_tests` target in the component's own
`BUILD.gn`, never individually in a top-level test target. For code under
`//brave/components/...`, add that `:unit_tests` target to
`test("brave_components_unittests")` in `components/BUILD.gn`. Add it to
`brave_unit_tests` in `test/BUILD.gn` only when the tests genuinely need
chrome/browser-layer test support (`//chrome/...`, profile or browser-process
scaffolding) that `brave_components_unittests` does not provide. NEVER flag an
existing dependency as a violation solely because it lives in
`brave_unit_tests`: many do for historical reasons.

**Why.** A test target owned by its component keeps the tests' deps next to the
code they test. `brave_components_unittests` is lighter than `brave_unit_tests`,
so component tests there build and run faster and cannot silently start
depending on the browser layer.

## Examples

Bad:

```gn
# test/BUILD.gn -> test("brave_unit_tests")
sources += [ "//brave/components/ai_chat/core/credential_manager_unittest.cc" ]
deps += [ "//brave/components/brave_vpn/browser/v2:unit_tests" ]
```

Good:

```gn
# components/brave_vpn/browser/v2/BUILD.gn
source_set("unit_tests") {
  testonly = true
  sources = [ "..._unittest.cc" ]
  deps = [ ... ]
}

# components/BUILD.gn -> test("brave_components_unittests")
deps += [ "//brave/components/brave_vpn/browser/v2:unit_tests" ]
```
