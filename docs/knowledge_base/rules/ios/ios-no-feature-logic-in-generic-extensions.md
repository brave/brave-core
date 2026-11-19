---
name: brave/ios-no-feature-logic-in-generic-extensions
description:
  'Feature-specific logic goes in feature-specific helpers, not in generic
  utility extensions of types such as URL, String or UIView.'
rubric: design
severity: should_fix
---

**Rule.** Do not add members that serve one feature to a generic utility
extension (`extension URL`, `extension String`, `extension UIView`, the files
under `Extensions/`). Put them in a helper that belongs to the feature.

**Why.** Generic extensions are visible to every user of the type; a feature's
logic there leaks into unrelated code, couples the shared module to the feature,
and is hard to find from the feature.

## Examples

Bad:

```swift
// URLExtensions.swift
extension URL {
  var isPlaylistSupportedSite: Bool { ... }
}
```

Good:

```swift
// PlaylistHelper.swift
enum PlaylistHelper {
  static func isSupportedSite(_ url: URL) -> Bool { ... }
}
```
