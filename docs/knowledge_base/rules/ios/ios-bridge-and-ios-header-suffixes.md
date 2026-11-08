---
name: brave/ios-bridge-and-ios-header-suffixes
description:
  'Objective-C wrapper headers exposing C++ types to Swift end in _bridge.h; iOS
  implementations of cross-platform interfaces end in _ios.h.'
rubric: readability
severity: nit
---

**Rule.** Name a new header by what it holds:

- a plain Objective-C wrapper that exposes C++ types to Swift: `<name>_bridge.h`
  (`protection_stats_tab_helper_bridge.h`);
- an iOS-specific implementation of a cross-platform interface: `<name>_ios.h`
  (`ads_client_ios.h`).

**Why.** The suffix tells the reader, and the build files, whether a header is
Swift-facing API or a C++ implementation detail.

## Examples

Bad:

```
ios/browser/brave_shields/protection_stats_tab_helper_ios.h  // ObjC wrapper.
ios/browser/api/ads/ads_client_bridge.h  // Implements brave_ads::AdsClient.
```

Good:

```
ios/browser/brave_shields/protection_stats_tab_helper_bridge.h
ios/browser/api/ads/ads_client_ios.h
```
