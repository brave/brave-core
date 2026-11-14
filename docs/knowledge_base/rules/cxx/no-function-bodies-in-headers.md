---
name: brave/no-function-bodies-in-headers
description:
  'Functions are defined in .cc files, not in headers; only simple snake_case
  accessors, test accessors and constexpr constants stay inline.'
rubric: readability
severity: should_fix
---

**Rule.** Declare functions and methods in the header and define them in the
`.cc` file. The exceptions are simple accessors, which are named `snake_case()`
and may be defined inline in the class, test accessors, and `constexpr`
constants. A method with any logic beyond returning a member, including a
one-line one, goes in the `.cc` file. Templates, which callers must see defined,
are outside this rule.

**Why.** A body in a header is compiled into every file that includes it, drags
its dependencies into the header, and recompiles them all whenever it changes.
Chromium's style guide keeps inline functions to simple accessors for this
reason.

## Examples

Bad:

```cpp
// rewards_protocol_handler.h
class RewardsProtocolHandler {
  static bool HandleURL(const GURL& url) {
    return url.scheme() == "rewards";
  }
};
```

Good:

```cpp
// rewards_protocol_handler.h
bool HandleRewardsProtocol(const GURL& url);

// rewards_protocol_handler.cc
bool HandleRewardsProtocol(const GURL& url) {
  return url.scheme() == "rewards";
}

// A simple accessor stays inline.
class RewardsService {
 public:
  bool is_enabled() const { return is_enabled_; }

 private:
  bool is_enabled_ = false;
};
```
