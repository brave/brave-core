---
name: brave/run-whole-test-file-when-fixing-a-test
description:
  A fix to a flaky or failing test is verified by running every test in its
  file, across all its fixtures, not only the test changed.
rubric: testing
severity: should_fix
---

**Rule.** When a change fixes a flaky or failing test (by changing the test, its
fixture, or the filter that disabled it), the author runs all the tests in that
file, across every fixture it defines, not only the test that changed. A fixture
or helper change in particular reaches every test using it. Ask for that run
when the change touches shared setup and its description reports only the single
test.

**Why.** Tests in one file share fixtures, helpers and globals, so a fix to one
of them can break, or start depending on, its neighbours. Running only the fixed
test misses those interactions until they fail on a bot.

## Examples

Bad:

```sh
# Only the test that was fixed.
npm run test -- brave_browser_tests --filter='FooBrowserTest.ShowsToolbar'
```

Good:

```sh
# Every fixture of foo_browsertest.cc.
npm run test -- brave_browser_tests \
  --filter='FooBrowserTest.*:FooWithBarBrowserTest.*'
```
