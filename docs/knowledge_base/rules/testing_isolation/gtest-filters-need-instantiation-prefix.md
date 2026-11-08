---
name: brave/gtest-filters-need-instantiation-prefix
description:
  A gtest filter entry for a parameterized suite includes the instantiation
  prefix and parameter suffix; the bare fixture name matches nothing.
rubric: testing
severity: should_fix
---

**Rule.** A filter entry naming a test of a value-parameterized suite (`TEST_P`,
instantiated with `INSTANTIATE_TEST_SUITE_P`) MUST include the instantiation
prefix and the parameter suffix, as in `All/MyCachingTest.Foo/*` or
`*/MyCachingTest.Foo/*`. Flag an entry in a `test/filters/` file, or a
`--gtest_filter`, that names such a suite as `MyCachingTest.Foo`.

**Why.** The full name of a parameterized test is
`<Instantiation>/<Suite>.<Test>/<Param>`, so the bare name matches no test, and
gtest treats a filter matching nothing as success: a positive filter runs
nothing and passes, and an exclusion in a filter file leaves the test running.

## Examples

Bad:

```
-MyCachingTest.NotifiesWhenReady
```

Good:

```
-All/MyCachingTest.NotifiesWhenReady/*
```
