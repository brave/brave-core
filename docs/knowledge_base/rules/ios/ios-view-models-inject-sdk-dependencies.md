---
name: brave/ios-view-models-inject-sdk-dependencies
description:
  'View models never import or reference SDK singletons; SDK dependencies are
  passed in through the initializer.'
rubric: testing
severity: blocking
---

**Rule.** A view model MUST NOT reach an SDK singleton (`BraveStoreSDK.shared`,
`Foo.shared` of an SDK module) or import the SDK only to do so. Take the SDK
dependency as an initializer parameter.

**Why.** A view model bound to a singleton cannot be tested with a fake SDK, and
its dependency is hidden from whoever creates it.

## Examples

Bad:

```swift
@Observable
final class OriginPaywallViewModel {
  private let store = BraveStoreSDK.shared
}
```

Good:

```swift
@Observable
final class OriginPaywallViewModel {
  private let store: BraveStoreSDK

  init(store: BraveStoreSDK) {
    self.store = store
  }
}
```
