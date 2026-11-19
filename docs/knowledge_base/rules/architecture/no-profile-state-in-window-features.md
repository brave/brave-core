---
name: brave/no-profile-state-in-window-features
description:
  'A browser-window-scoped feature does not store profile state, since several
  windows can share one profile.'
rubric: design
severity: blocking
---

**Rule.** A feature owned by `BrowserWindowFeatures` MUST NOT keep state that
belongs to the profile (data, settings or caches every window of the profile
should see). Keep that state in a profile-scoped keyed service, and have the
window feature use the service.

**Why.** Several windows can share a profile; per-profile state stored in one
window's feature is duplicated in the others, diverges between them, and is lost
when that window closes.

## Examples

Bad:

```cpp
class FooWindowFeature {
  std::vector<GURL> profile_bookmarks_cache_;  // per-profile data
};
```

Good:

```cpp
class FooWindowFeature {
  raw_ref<FooBookmarksService> bookmarks_service_;  // profile-scoped
};
```
