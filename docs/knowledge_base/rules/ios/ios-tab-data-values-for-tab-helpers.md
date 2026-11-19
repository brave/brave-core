---
name: brave/ios-tab-data-values-for-tab-helpers
description:
  'Tab-associated data is stored with the TabDataValues extension pattern, never
  in stored properties or associated objects.'
rubric: design
severity: blocking
---

**Rule.** To keep data per tab (a tab helper or its state), declare it as a
property in an `extension TabDataValues`, backed by a `TabDataKey`. Do not add
stored properties to the tab type for it, and NEVER attach it with
`objc_setAssociatedObject`.

**Why.** `TabDataValues` ties the data's lifetime to the tab's, so it is created
and released with the tab; stored properties couple every feature to the tab
type, and associated objects escape that lifecycle management.

## Examples

Bad:

```swift
private var playlistHelperKey: UInt8 = 0

extension TabState {
  var playlistHelper: PlaylistTabHelper? {
    objc_getAssociatedObject(self, &playlistHelperKey) as? PlaylistTabHelper
  }
}
```

Good:

```swift
extension TabDataValues {
  private struct PlaylistTabHelperKey: TabDataKey {
    static var defaultValue: PlaylistTabHelper?
  }

  var playlist: PlaylistTabHelper? {
    get { self[PlaylistTabHelperKey.self] }
    set { self[PlaylistTabHelperKey.self] = newValue }
  }
}
```
