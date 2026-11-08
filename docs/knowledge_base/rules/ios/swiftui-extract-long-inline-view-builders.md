---
name: brave/swiftui-extract-long-inline-view-builders
description:
  'An inline SwiftUI view builder block longer than a few lines is extracted
  into a named View struct or computed property.'
rubric: readability
severity: nit
---

**Rule.** When an inline view builder block (the content of a stack, `List`,
`ForEach`, `Section`, `.overlay`, `.sheet`, a `Button` label and the like) grows
beyond a few lines, move it into a named `View` struct, or into a computed
property or method of the enclosing view, and use it by name.

**Why.** Named subviews keep `body` readable as an outline of the screen, and
can be reused.

## Examples

Bad:

```swift
var body: some View {
  List {
    ForEach(items) { item in
      HStack {
        Image(braveSystemName: item.icon)
        VStack(alignment: .leading) {
          Text(item.title)
          Text(item.subtitle)
            .foregroundStyle(Color(braveSystemName: .textSecondary))
        }
      }
    }
  }
}
```

Good:

```swift
var body: some View {
  List {
    ForEach(items) { item in
      ItemRow(item: item)
    }
  }
}
```
