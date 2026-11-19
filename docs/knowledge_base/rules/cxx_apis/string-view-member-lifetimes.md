---
name: brave/string-view-member-lifetimes
description:
  'A std::string_view data member is a lifetime bug when it can be initialized
  from a temporary or non-static string; members initialized from string
  literals or static constants such as pref keys are safe and must not be
  flagged.'
rubric: safety
severity: blocking
---

**Rule.** Before flagging a `std::string_view` data member, check what it is
initialized from.

Flag it when:

- it is initialized from a `std::string` temporary or a function's return value;
- it lives in a long-lived object (a singleton, a `KeyedService`) and is
  initialized from a string that is not static;
- the constructor takes a `std::string_view` and stores it, so a caller may pass
  a temporary.

NEVER flag it when:

- it is initialized from an `inline constexpr char[]` constant (such as a pref
  key `prefs::kMyPref`), a `kFeatureName` literal or another compile-time string
  with static storage duration;
- it is initialized from a string literal directly;
- it is a member of a short-lived stack object that the caller's string clearly
  outlives.

For a flagged member, store a `std::string` instead.

**Why.** A `std::string_view` does not own its characters. When the string it
was built from is destroyed first, every later read is a use-after-free. Static
strings live for the whole program, so views of them are safe.

## Examples

Bad:

```cpp
class Config {
 public:
  explicit Config(std::string_view name) : name_(name) {}
 private:
  std::string_view name_;  // dangles if a temporary was passed
};
```

Good:

```cpp
class MyMetrics {
  std::string_view histogram_name_ = kHistogramName;  // static storage
  std::string_view pref_path_ = prefs::kMyPref;       // static storage
};
```
