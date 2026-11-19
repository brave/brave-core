---
name: brave/check-deref-only-for-nullable-pointers
description:
  'CHECK_DEREF converts a pointer that could be null for a const raw_ref<T>
  field, instead of *ptr; it is not added for pointers that are never null in
  practice.'
rubric: safety
severity: should_fix
---

**Rule.** When a `const raw_ref<T>` field is initialized from a pointer that
genuinely could be null, wrap it in `CHECK_DEREF(ptr)` rather than writing
`*ptr`. Do not add `CHECK_DEREF` to a pointer that effectively never returns
null in practice, such as `browser->GetProfile()`: store the dependency as it
comes (take a `T&` in the constructor, or dereference directly) without the
extra assertion.

**Why.** `CHECK_DEREF` asserts non-null and converts to a reference, while
`*ptr` on null is undefined behavior. On a pointer that is never null it only
adds a redundant runtime check that upstream code does not do.

## Examples

Bad:

```cpp
explicit MyService(Profile& profile) : prefs_(*profile.GetPrefs()) {}

explicit MyFeature(Browser* browser)
    : profile_(CHECK_DEREF(browser->GetProfile())) {}
```

Good:

```cpp
explicit MyService(Profile& profile)
    : prefs_(CHECK_DEREF(profile.GetPrefs())) {}

explicit MyFeature(Browser* browser) : profile_(*browser->GetProfile()) {}
```
