---
name: brave/std-shared-ptr-is-banned
description:
  'std::shared_ptr is banned in Chromium code; use
  base::RefCounted/scoped_refptr when shared ownership is truly needed, or
  unique ownership.'
rubric: design
severity: blocking
---

**Rule.** NEVER use `std::shared_ptr` (or `std::make_shared`). When shared
ownership is truly needed, use `base::RefCounted` / `scoped_refptr`; otherwise
restructure the code to use unique ownership.

**Why.** `std::shared_ptr` is on Chromium's list of banned C++ features. Shared
ownership also makes destruction order hard to reason about, so unique ownership
is the first choice.

## Examples

Bad:

```cpp
std::shared_ptr<Cache> cache_ = std::make_shared<Cache>();
```

Good:

```cpp
std::unique_ptr<Cache> cache_ = std::make_unique<Cache>();
```
