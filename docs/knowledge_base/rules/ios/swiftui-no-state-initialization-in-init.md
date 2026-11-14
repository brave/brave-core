---
name: brave/swiftui-no-state-initialization-in-init
description:
  'SwiftUI @State properties are never assigned in init with
  State(wrappedValue:); the initial value is passed in and assigned in onAppear
  or task.'
rubric: correctness
severity: blocking
---

**Rule.** NEVER set a `@State` property in a view's `init`
(`self._value = State(wrappedValue: value)` or `State(initialValue:)`). Store
the initial value in a plain property and assign it to the `@State` in an
`.onAppear` or `.task` modifier.

**Why.** SwiftUI owns `@State` storage and keeps it across re-creations of the
view struct; assigning it in `init` can cause undefined behaviour on later body
computations.

## Examples

Bad:

```swift
struct ContentView: View {
  @State private var value: Int
  init(value: Int) {
    self._value = State(wrappedValue: value)
  }
}
```

Good:

```swift
struct ContentView: View {
  var initialValue: Int
  @State private var value: Int?

  var body: some View {
    Color.black
      .onAppear {
        value = initialValue
      }
  }
}
```
