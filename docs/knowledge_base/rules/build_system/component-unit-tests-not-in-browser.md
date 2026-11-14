---
name: brave/component-unit-tests-not-in-browser
description:
  'Unit tests for code in //brave/components must live alongside that code,
  never in //brave/browser.'
rubric: testing
severity: blocking
---

**Rule.** A unit test of code under `//brave/components/...` MUST live in the
same component, next to the code it tests, and MUST NOT be placed under
`//brave/browser`. When it needs the content layer, use
`content::RenderViewHostTestHarness` in the component's tests.

**Why.** Tests next to their code are built and run with the component, and
moving them into `//brave/browser` gives component tests browser-layer
dependencies that the component itself must not have.

## Examples

Bad:

```text
browser/ai_chat/associated_link_content_unittest.cc
  # tests components/ai_chat/content/browser/associated_link_content.cc
```

Good:

```gn
# components/ai_chat/content/browser/BUILD.gn
source_set("unit_tests") {
  sources = [ "associated_link_content_unittest.cc" ]
}
```
