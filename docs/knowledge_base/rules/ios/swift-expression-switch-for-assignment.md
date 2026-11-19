---
name: brave/swift-expression-switch-for-assignment
description:
  'In Swift, a value chosen by a switch is assigned from an expression-level
  switch, not from assignments in each case.'
rubric: readability
severity: nit
---

**Rule.** When every case of a `switch` only produces the value of one variable,
write it as a `switch` expression (`let title = switch state { ... }`) instead
of declaring the variable first and assigning it in each case.

**Why.** The expression form states once what is being computed, lets the
variable be a `let`, and makes the compiler check that every case yields a
value.

## Examples

Bad:

```swift
let title: String
switch state {
case .loading: title = "Loading..."
case .loaded: title = "Done"
case .error: title = "Failed"
}
```

Good:

```swift
let title = switch state {
  case .loading: "Loading..."
  case .loaded: "Done"
  case .error: "Failed"
}
```
