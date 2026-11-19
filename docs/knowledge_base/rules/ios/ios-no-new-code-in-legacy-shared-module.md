---
name: brave/ios-no-new-code-in-legacy-shared-module
description:
  'No new functionality is added to the legacy Shared Swift module; new shared
  code gets a new module.'
rubric: design
severity: should_fix
---

**Rule.** Do not add new types, functions, extensions or files to the `Shared`
module (`ios/brave-ios/Sources/Shared/`). Put new shared code in a new module.
Using existing `Shared` code, and fixing it, remains fine.

**Why.** `Shared` is a legacy grab-bag that most modules depend on; each
addition makes it harder to retire and couples unrelated code through it.

## Examples

Bad:

```swift
// Sources/Shared/Extensions/URLExtensions.swift
extension URL {
  public var isBraveSearchURL: Bool { ... }  // New functionality.
}
```

Good:

```swift
// Sources/BraveSearch/URL+BraveSearch.swift, in a new module
extension URL {
  public var isBraveSearchURL: Bool { ... }
}
```
