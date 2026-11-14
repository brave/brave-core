---
name: brave/null-check-services-unavailable-in-incognito
description:
  A service reached through a JNI bridge or native code must be null-checked
  where it is used, because incognito profiles may not have it.
rubric: correctness
severity: blocking
---

**Rule.** A service obtained for a profile and used from a bridge or other
native code (for example a keyed service such as `ViewCounterService` held by a
JNI bridge) MUST be null-checked at the point of use, even when upstream logic
is supposed to keep incognito profiles from reaching that code.

**Why.** Many Brave services are not created for incognito profiles, so their
factories return null for them. Upstream logic that keeps such profiles away is
Chromium's, and can change under a rebase; without a check at the point of use,
the first path that slips through dereferences null.

## Examples

Bad:

```cpp
void NTPBackgroundImagesBridge::GetCurrentWallpaperForDisplay() {
  view_counter_service_->GetCurrentWallpaperForDisplay();
}
```

Good:

```cpp
void NTPBackgroundImagesBridge::GetCurrentWallpaperForDisplay() {
  if (!view_counter_service_) {
    return;
  }
  view_counter_service_->GetCurrentWallpaperForDisplay();
}
```
