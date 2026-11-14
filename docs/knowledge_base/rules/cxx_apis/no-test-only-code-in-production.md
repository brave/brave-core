---
name: brave/no-test-only-code-in-production
description:
  'Production code must not gain branches or behaviour that exist only to make
  tests pass; tests use test infrastructure instead. Flag only code that is
  certainly test-only, and never thin ForTesting() accessors of internal
  features.'
rubric: design
severity: blocking
---

**Rule.** Production code MUST NOT change its behaviour for tests: no branch,
flag, alternative code path or relaxed check that only tests need. Make the test
use test infrastructure instead, such as a `kHostResolverRules` switch in
`SetUpCommandLine()`, a fake or mock injected through the existing interface, or
a feature or pref override.

Flag a violation ONLY when the code certainly exists solely for tests, as shown
by `CHECK_IS_TEST()`, `#if defined(UNIT_TEST)`, a `_for_testing`/`ForTesting`
suffix, or a comment saying it is there for tests. NEVER flag:

- ordinary defensive code, such as handling empty, null or default values, reset
  paths or cleanup behaviour;
- thin `ForTesting()` accessors that expose an internal `base::Feature` or
  similar without affecting production behaviour.

When uncertain, do not flag.

**Why.** A test-only path in production code is shipped to every user, can be
triggered outside tests, and means the tests exercise behaviour production does
not have.

## Examples

Bad:

```cpp
GURL FooService::GetServerUrl() const {
  if (use_local_server_for_testing_) {
    return GURL("http://127.0.0.1:8080");
  }
  return GURL(kServerUrl);
}
```

Good:

```cpp
void FooBrowserTest::SetUpCommandLine(base::CommandLine* command_line) {
  command_line->AppendSwitchASCII(network::switches::kHostResolverRules,
                                  "MAP * 127.0.0.1");
}
```
