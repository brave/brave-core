---
name: brave/swift-for-where-over-guard-continue
description:
  'A Swift for-in loop that skips elements with a leading guard ... else {
  continue } uses a where clause instead.'
rubric: readability
severity: nit
---

**Rule.** When a `for`-`in` loop starts with
`guard <condition> else { continue }` to skip elements, move the condition into
a `where` clause: `for item in items where item.isEnabled { ... }`.

**Why.** The `where` clause states the filter in the loop header, where the
reader looks for it, and removes a level of control flow from the body.

## Examples

Bad:

```swift
for item in items {
  guard item.isEnabled else { continue }
  process(item)
}
```

Good:

```swift
for item in items where item.isEnabled {
  process(item)
}
```
