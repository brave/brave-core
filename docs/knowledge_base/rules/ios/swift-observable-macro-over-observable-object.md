---
name: brave/swift-observable-macro-over-observable-object
description:
  'New Swift observable models use the @Observable macro (Observation
  framework), not ObservableObject with @Published.'
rubric: design
severity: should_fix
---

**Rule.** Declare observable model types with the `@Observable` macro and plain
stored properties. Do not add new `ObservableObject` conformances with
`@Published` properties.

**Why.** `@Observable` is the modern replacement for the Combine-based
`ObservableObject`: it needs less boilerplate, and SwiftUI tracks which
properties a view reads, so a change to one property does not invalidate every
view observing the model.

## Examples

Bad:

```swift
class ViewModel: ObservableObject {
  @Published var items: [Item] = []
}
```

Good:

```swift
@Observable
class ViewModel {
  var items: [Item] = []
}
```
