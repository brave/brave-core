---
name: brave/swift-disfavored-overload-for-backports
description:
  'Swift backports of newer APIs are declared with @_disfavoredOverload and
  @available(iOS, obsoleted:), so they disappear once the deployment target is
  raised.'
rubric: design
severity: should_fix
---

**Rule.** To make a newer system API callable on older iOS versions, declare the
fallback with the same name, annotated with both `@_disfavoredOverload` and
`@available(iOS, obsoleted: N)`, where `N` is the version that introduced the
API. Do not give the fallback a different name, or wrap each call site in
`if #available`.

**Why.** Callers use the real API's name; on new systems the compiler prefers
the system API over the disfavored overload, and once the deployment target
reaches `N` the obsoleted fallback stops compiling, so it is removed rather than
left behind.

## Examples

Bad:

```swift
extension View {
  func compatScrollBounceBehavior(
    _ behavior: ScrollBounceBehavior
  ) -> some View {
    if #available(iOS 17, *) {
      return AnyView(scrollBounceBehavior(behavior))
    }
    return AnyView(self)
  }
}
```

Good:

```swift
@_disfavoredOverload
@available(iOS, obsoleted: 17.0)
extension View {
  func scrollBounceBehavior(_ behavior: ScrollBounceBehavior) -> some View {
    // Fallback implementation
    self
  }
}
```
