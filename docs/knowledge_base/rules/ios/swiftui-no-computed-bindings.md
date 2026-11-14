---
name: brave/swiftui-no-computed-bindings
description:
  'SwiftUI code does not build Binding(get:set:) closures to transform state or
  run side effects; it uses a helper property, a subscript, onChange or an
  observable model.'
rubric: perf
severity: should_fix
---

**Rule.** Do not pass `Binding(get:set:)` to a view to transform state or to run
a side effect when the value changes. To transform, add a computed property (or
a subscript, when it needs arguments) on the type or model and bind to it with
`$value.isPresented`. For side effects, bind the state directly and react with
`.onChange(of:)`, or move the state into an `@Observable` model and do the work
in the property's `didSet`.

**Why.** A closure-based binding is a new value on every body evaluation, so
SwiftUI cannot tell whether it changed and rebuilds the hierarchy that uses it
unnecessarily.

## Examples

Bad:

```swift
Toggle("Setting", isOn: Binding(
  get: { isEnabled },
  set: { newValue in
    isEnabled = newValue
    performSomeWork()
  }
))
```

Good:

```swift
Toggle("Setting", isOn: $isEnabled)
  .onChange(of: isEnabled) { _, newValue in
    if newValue {
      performSomeWork()
    }
  }
```
