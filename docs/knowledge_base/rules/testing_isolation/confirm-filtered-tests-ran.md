---
name: brave/confirm-filtered-tests-ran
description:
  A filtered test run counts as a pass only once the tests are confirmed to be
  in the binary and the run shows them executing.
rubric: testing
severity: should_fix
---

**Rule.** Before reporting that a fixed or re-enabled test passes, confirm that
the binary contains it (`--gtest_list_tests --gtest_filter=...` lists it) and
that the run printed a `[ RUN ]`/`[ OK ]` pair with a non-zero duration for each
test. When the tests are missing because their target is behind a build flag
(such as `enable_ai_chat`, off with `is_brave_origin_branded=true`), build a
separate output directory with the flag flipped instead of reconfiguring a
shared one. Question a change whose evidence of passing is a filtered run that
could have matched nothing.

**Why.** A filter that matches no test prints a warning, reports
`SUCCESS: all tests passed.` and exits 0, which looks like a real pass in a log
or a CI summary. It is the normal outcome when the test's target is compiled out
of the build directory used.

## Examples

Bad:

```
WARNING: No matching tests to run.
SUCCESS: all tests passed.
Tests took 0 seconds.
```

Good:

```sh
# Lists the expected tests; empty output means they are not in this binary.
./out/<dir>/brave_browser_tests --gtest_list_tests \
  --gtest_filter='AIChatPDFOCRBrowserTest.*'
```
