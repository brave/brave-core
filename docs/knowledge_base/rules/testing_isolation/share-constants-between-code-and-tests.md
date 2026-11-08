---
name: brave/share-constants-between-code-and-tests
description:
  A test does not redefine a constant the production code defines; both use one
  definition exposed in a shared header.
rubric: design
severity: should_fix
---

**Rule.** When a test needs a value the production code also defines (a delay, a
limit, a key, a URL), it uses the production constant, moved to a header as an
`inline constexpr` if it was local to a `.cc` file. Tests MUST NOT define their
own copy of that constant.

**Why.** A duplicated constant drifts: when the production value changes, the
test keeps the old one and either fails for the wrong reason or, worse, keeps
passing while no longer testing the real value.

## Examples

Bad:

```cpp
// production.cc
constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);
// production_unittest.cc
constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);
```

Good:

```cpp
// constants.h, included by both production.cc and production_unittest.cc.
inline constexpr base::TimeDelta kCleanupDelay = base::Seconds(30);
```
