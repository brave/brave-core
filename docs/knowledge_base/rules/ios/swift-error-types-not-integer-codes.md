---
name: brave/swift-error-types-not-integer-codes
description:
  'Swift code represents errors as Error enums or structs, not integer error
  codes.'
rubric: design
severity: should_fix
---

**Rule.** Model failures as a type conforming to `Error`: an enum with a case
per failure (carrying the underlying error where there is one), or a struct. Do
not represent them as `Int`-backed enums or integer error codes.

**Why.** An `Error` type is checked by the compiler, `switch` over it is
exhaustive, and its cases carry their context, which makes failures easier to
handle and to debug than bare numbers.

## Examples

Bad:

```swift
enum FetchError: Int {
  case networkError = 1
  case parseError = 2
}
```

Good:

```swift
enum FetchError: Error {
  case networkFailure(URLError)
  case parseFailure(DecodingError)
}
```
