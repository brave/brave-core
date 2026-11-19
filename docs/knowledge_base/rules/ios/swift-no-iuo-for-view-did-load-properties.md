---
name: brave/swift-no-iuo-for-view-did-load-properties
description:
  'Properties initialized in viewDidLoad are not implicitly unwrapped optionals;
  they are lazy, safely unwrapped optionals, or set in the initializer.'
rubric: safety
severity: blocking
---

**Rule.** NEVER declare a property that `viewDidLoad` initializes as an
implicitly unwrapped optional (`var tableView: UITableView!`). Use a `lazy var`,
an optional (`UITableView?`) unwrapped safely at each use, or a non-optional
property initialized in the initializer.

**Why.** An implicitly unwrapped optional crashes when it is read while still
`nil`, and code that runs before `viewDidLoad` (an initializer, a property
observer, a delegate callback) can read it.

## Examples

Bad:

```swift
var tableView: UITableView!  // Crashes if read before viewDidLoad.
```

Good:

```swift
lazy var tableView = UITableView()
```
