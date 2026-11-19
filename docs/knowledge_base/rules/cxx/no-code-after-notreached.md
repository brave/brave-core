---
name: brave/no-code-after-notreached
description:
  'No executable statement follows NOTREACHED(), which is fatal in all builds
  and ends control flow.'
rubric: correctness
severity: should_fix
---

**Rule.** Do not write statements after `NOTREACHED()` in the same block: no
fallback `return`, `break` or assignment. The compiler treats them as dead code,
and they never run.

**Why.** `NOTREACHED()` is `[[noreturn]]` and fatal in every build. Code after
it suggests to the reader that execution may continue there, which it cannot.

## Examples

Bad:

```cpp
default:
  NOTREACHED();
  return kDefaultValue;
```

Good:

```cpp
default:
  NOTREACHED();
```
