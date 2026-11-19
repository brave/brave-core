---
name: brave/swift-no-redundant-main-actor-on-task
description:
  'A Task created inside a @MainActor-isolated context inherits the isolation,
  so it does not repeat @MainActor.'
rubric: readability
severity: nit
---

**Rule.** Do not write `Task { @MainActor in ... }` inside a method or type that
is already `@MainActor`-isolated (a `@MainActor` class, a `UIViewController`
subclass). Write `Task { ... }`.

**Why.** A `Task` inherits the actor isolation of the context that creates it,
so the annotation changes nothing and suggests that the surrounding code is not
on the main actor.

## Examples

Bad:

```swift
@MainActor
class MyViewController: UIViewController {
  func refresh() {
    Task { @MainActor in
      tableView.reloadData()
    }
  }
}
```

Good:

```swift
@MainActor
class MyViewController: UIViewController {
  func refresh() {
    Task {
      let data = await fetchData()
      tableView.reloadData()  // Already on the main actor.
    }
  }
}
```
