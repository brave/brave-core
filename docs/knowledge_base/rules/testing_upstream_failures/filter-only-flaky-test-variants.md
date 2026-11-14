---
name: brave/filter-only-flaky-test-variants
description:
  A filter file disables only the parameterized variants LUCI Analysis shows
  flaky, never a wildcard that also disables stable variants.
rubric: testing
severity: should_fix
---

**Rule.** In a filter file, do NOT use a wildcard entry (such as
`-SomeParameterizedTest/*`) that disables stable variants of a test along with
the flaky ones. Check the LUCI Analysis data of each variant individually and
list only the specific variants that are actually flaky, keeping the stable ones
enabled.

**Why.** A wildcard silently drops the coverage of every stable variant, and
keeps them disabled for as long as the entry stays.

## Examples

Bad:

```
-SomeParameterizedTest/*
```

Good:

```
-SomeParameterizedTest/0
-SomeParameterizedTest/2
-SomeParameterizedTest/3
# Variant /1 is stable upstream - keep it enabled
```
