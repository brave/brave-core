---
name: brave/swift-set-for-membership-tests
description:
  'A Swift collection used mainly for contains() lookups is a Set, not an Array.'
rubric: perf
severity: nit
---

**Rule.** Declare a collection whose main use is `contains(_:)` as a `Set`
(`let allowedTypes: Set<String> = [...]`), not as an `Array`. Keep an `Array`
when order or duplicates matter or when the collection is mainly iterated.

**Why.** `Set.contains` is O(1); `Array.contains` scans every element, O(n).

## Examples

Bad:

```swift
let allowedTypes: [String] = ["image", "video", "audio"]
if allowedTypes.contains(type) { ... }
```

Good:

```swift
let allowedTypes: Set<String> = ["image", "video", "audio"]
if allowedTypes.contains(type) { ... }
```
