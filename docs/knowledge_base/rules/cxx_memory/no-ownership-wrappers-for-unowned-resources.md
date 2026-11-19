---
name: brave/no-ownership-wrappers-for-unowned-resources
description:
  'A class must never wrap a resource it does not own in an owning wrapper
  (std::unique_ptr, scoped_refptr, a platform scoper).'
rubric: safety
severity: blocking
---

**Rule.** NEVER put a pointer or handle that the class does not own into an
ownership wrapper: a `std::unique_ptr` (including through `base::WrapUnique()`
or `.reset(ptr)`), a `scoped_refptr`, or a scoper that releases the resource on
destruction. Keep unowned resources as non-owning references (`raw_ptr<T>`,
`raw_ref<T>`, `T*`, `T&`) and leave their release to the real owner.

**Why.** The wrapper frees the resource when it goes out of scope while the real
owner still uses it and frees it again later, which gives a use-after-free or a
double free. This is a common source of crashes.

## Examples

Bad:

```cpp
// `prefs` is owned by the profile.
FooService::FooService(PrefService* prefs) : prefs_(prefs) {}
std::unique_ptr<PrefService> prefs_;
```

Good:

```cpp
FooService::FooService(PrefService* prefs) : prefs_(prefs) {}
raw_ptr<PrefService> prefs_;
```
