---
name: brave/ios-set-properties-before-setup-in-view-did-load
description:
  'In viewDidLoad, properties are set before the setup methods that depend on
  them are called.'
rubric: correctness
severity: should_fix
---

**Rule.** In `viewDidLoad`, assign each property before calling any setup method
that reads it. When adding or reordering statements in `viewDidLoad`, check that
no setup call now runs before a property it uses is set.

**Why.** A setup method that reads a property not yet set configures the view
with `nil` or stale state; such ordering bugs are common and hard to debug.

## Examples

Bad:

```swift
override func viewDidLoad() {
  super.viewDidLoad()
  setupTableView()  // Uses dataSource, which isn't set yet.
  dataSource = makeDataSource()
}
```

Good:

```swift
override func viewDidLoad() {
  super.viewDidLoad()
  dataSource = makeDataSource()
  setupTableView()
}
```
