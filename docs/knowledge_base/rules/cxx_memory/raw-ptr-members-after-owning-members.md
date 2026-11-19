---
name: brave/raw-ptr-members-after-owning-members
description:
  'In class declarations, unowned raw_ptr<> members are placed after the owning
  members such as std::unique_ptr<>.'
rubric: readability
severity: nit
---

**Rule.** Declare a class's owning members (such as `std::unique_ptr<>`) first
and its unowned `raw_ptr<>` members after them, rather than mixing the two. The
`base::WeakPtrFactory`, when there is one, still comes last.

**Why.** It follows Chromium convention and makes the ownership of each member
visible at a glance.

## Examples

Bad:

```cpp
class MyService {
  raw_ptr<PrefService> prefs_;
  std::unique_ptr<Fetcher> fetcher_;
  raw_ptr<ProfileManager> profile_manager_;
};
```

Good:

```cpp
class MyService {
  std::unique_ptr<Fetcher> fetcher_;
  raw_ptr<PrefService> prefs_;
  raw_ptr<ProfileManager> profile_manager_;
};
```
