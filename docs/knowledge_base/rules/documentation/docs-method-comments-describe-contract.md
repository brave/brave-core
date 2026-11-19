---
name: brave/docs-method-comments-describe-contract
description:
  'Method documentation, especially on public methods in headers, describes the
  contract: behavior, parameters, return values, gotchas, restrictions and
  exceptions.'
rubric: readability
severity: should_fix
---

**Rule.** Document a method's contract, above all for public methods in headers:
what it does, what its parameters mean, what it returns (including when it
fails), and its gotchas, restrictions (such as threading) and exceptions. A
comment that only restates the name is not documentation.

**Why.** Callers rely on the contract, not on the implementation; without it
they guess, and the guesses become bugs when the implementation changes.

## Examples

Bad:

```cpp
// Gets the publisher info.
PublisherInfo GetPublisherInfo(const std::string& id);
```

Good:

```cpp
// Returns publisher info for `id`, or nullopt if not found in the local
// database. Does not trigger a network fetch. Must be called on the UI
// thread. The returned info may be stale if the publisher hasn't been
// visited recently.
std::optional<PublisherInfo> GetPublisherInfo(const std::string& id);
```
