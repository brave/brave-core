---
name: brave/swift-max-by-over-sorted-first
description:
  'Swift code takes the largest or smallest element with max(by:) or min(by:),
  not by sorting and taking .first or .last.'
rubric: perf
severity: nit
---

**Rule.** To find the extreme element of a collection, call `max(by:)`,
`min(by:)`, `max()` or `min()`. Do not sort the collection and take `.first` or
`.last` of the result.

**Why.** `sorted()` sorts the whole collection, O(n log n), and allocates a new
array, only to keep one element; `max(by:)` and `min(by:)` find it in one O(n)
pass and state the intent directly.

## Examples

Bad:

```swift
let newest = items.sorted(by: { $0.date > $1.date }).first
```

Good:

```swift
let newest = items.max(by: { $0.date < $1.date })
```
