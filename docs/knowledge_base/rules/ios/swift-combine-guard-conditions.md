---
name: brave/swift-combine-guard-conditions
description:
  'Consecutive Swift guard statements with the same else branch are combined
  into one guard with comma-separated conditions.'
rubric: readability
severity: nit
---

**Rule.** When consecutive `guard` statements exit through the same `else`
branch, merge them into one `guard` with comma-separated conditions:
`guard let a = optionalA, let b = optionalB else { return }`. Keep them separate
when their `else` branches differ.

**Why.** One `guard` states the function's preconditions in one place and
removes the repeated `else` blocks.

## Examples

Bad:

```swift
guard let a = optionalA else { return }
guard let b = optionalB else { return }
```

Good:

```swift
guard let a = optionalA, let b = optionalB else { return }
```
