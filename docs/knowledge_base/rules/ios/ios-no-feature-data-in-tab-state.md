---
name: brave/ios-no-feature-data-in-tab-state
description:
  'TabState holds only core tab state; feature-specific state lives in a Tab
  Helper stored with TabDataValues.'
rubric: design
severity: should_fix
---

**Rule.** Do not add a property for a feature's state to `TabState` or its
implementations (`TabStateImpl`, `ChromiumTabState`, `AnyTabState`). Keep that
state in the feature's Tab Helper, stored through `TabDataValues`.

**Why.** `TabState` is the core model every tab implementation provides; each
feature property added there grows that model and every implementation of it,
for state only one feature reads.

## Examples

Bad:

```swift
protocol TabState {
  // ...
  var translationLanguage: String? { get set }
}
```

Good:

```swift
extension TabDataValues {
  var translateHelper: BraveTranslateTabHelper? {
    get { self[BraveTranslateTabHelperKey.self] }
    set { self[BraveTranslateTabHelperKey.self] = newValue }
  }
}
```
