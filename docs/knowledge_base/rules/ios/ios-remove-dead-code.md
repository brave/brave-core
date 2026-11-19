---
name: brave/ios-remove-dead-code
description:
  'Dead code, such as commented-out code and unreachable or disabled branches,
  is removed before submitting.'
rubric: readability
severity: should_fix
---

**Rule.** Do not submit dead code: commented-out code, branches that can never
run (`if false`, `#if false`, `#if 0`), and functions or properties left without
callers by the change. Delete them; version control keeps the history.

**Why.** Dead code misleads readers about what runs, and goes stale against the
code around it.

## Examples

Bad:

```swift
func reload() {
  // tableView.reloadData()
  // updateHeader()
  collectionView.reloadData()
}
```

Good:

```swift
func reload() {
  collectionView.reloadData()
}
```
