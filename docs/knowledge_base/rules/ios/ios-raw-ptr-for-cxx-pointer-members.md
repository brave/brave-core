---
name: brave/ios-raw-ptr-for-cxx-pointer-members
description:
  'A C++ class in iOS code stores a pointer to another object as raw_ptr<T>, not
  as a raw T*.'
rubric: safety
severity: should_fix
---

**Rule.** Declare a pointer member of a C++ class or struct as `raw_ptr<T>`
(`raw_ptr<ProfileIOS> profile_;`), not `T*`. This concerns C++ class members,
not Objective-C `@property` declarations or method parameters.

**Why.** `raw_ptr<T>` enables Chromium's pointer safety checks (MiraclePtr),
which turn a use-after-free through the member into a safe crash instead of
memory corruption.

## Examples

Bad:

```cpp
class AdsServiceImplIOS : public AdsService {
  PrefService* prefs_;
};
```

Good:

```cpp
class AdsServiceImplIOS : public AdsService {
  raw_ptr<PrefService> prefs_;
};
```
