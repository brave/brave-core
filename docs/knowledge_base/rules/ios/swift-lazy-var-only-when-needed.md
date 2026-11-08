---
name: brave/swift-lazy-var-only-when-needed
description:
  'A Swift lazy var is used only to defer costly creation from init time, or
  when the initializer needs self.'
rubric: readability
severity: nit
---

**Rule.** Declare a property `lazy var` only when its creation must be deferred
to avoid work at init time, or when its initial value refers to `self`.
Otherwise give it a plain `let` with its initial value.

**Why.** `lazy` makes the property mutable and creates it on first access, at a
moment that is harder to predict; when neither deferral nor `self` is needed,
that cost buys nothing.

## Examples

Bad:

```swift
lazy var titleLabel = UILabel()  // Cheap, does not need self.
```

Good:

```swift
let titleLabel = UILabel()

lazy var dataSource = makeDataSource(for: self.tableView)  // Needs self.
```
