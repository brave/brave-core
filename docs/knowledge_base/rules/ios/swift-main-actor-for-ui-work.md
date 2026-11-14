---
name: brave/swift-main-actor-for-ui-work
description:
  'Swift methods and types that perform UI work are marked @MainActor, so that
  async callers must await them on the main actor.'
rubric: correctness
severity: should_fix
---

**Rule.** Mark a Swift method or type that touches UIKit or SwiftUI state as
`@MainActor`. Do not leave such a method non-isolated and rely on each caller to
hop to the main thread.

**Why.** With `@MainActor` on the declaration, the compiler makes every async
caller `await` it on the main actor, so UI updates cannot run on a background
executor. Without it, the method runs wherever its caller does, and each UI call
inside it needs its own `await`.

## Examples

Bad:

```swift
func updateUI() async {
  let data = await fetchData()
  await tableView.reloadData()  // Every UI call needs an await.
}
```

Good:

```swift
@MainActor
func updateUI() async {
  let data = await fetchData()
  tableView.reloadData()  // The method is MainActor-isolated.
}
```
