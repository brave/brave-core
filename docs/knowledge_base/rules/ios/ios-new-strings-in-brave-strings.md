---
name: brave/ios-new-strings-in-brave-strings
description:
  'New user-facing iOS strings are added to BraveStrings, not to the legacy
  SharedStrings.swift.'
rubric: design
severity: should_fix
---

**Rule.** Declare a new user-facing string in `BraveStrings`
(`ios/brave-ios/Sources/BraveStrings/`). Do not add strings to
`ios/brave-ios/Sources/Shared/SharedStrings.swift`, which holds legacy and
shared strings only.

**Why.** `SharedStrings.swift` belongs to the legacy `Shared` module; adding to
it grows the module that is being retired.

## Examples

Bad:

```swift
// Sources/Shared/SharedStrings.swift
extension Strings {
  public static let playlistTitle = NSLocalizedString(
    "PlaylistTitle", bundle: .module, value: "Playlist", comment: "")
}
```

Good:

```swift
// Sources/BraveStrings/BraveStrings.swift
extension Strings {
  public static let playlistTitle = NSLocalizedString(
    "PlaylistTitle", bundle: .module, value: "Playlist", comment: "")
}
```
