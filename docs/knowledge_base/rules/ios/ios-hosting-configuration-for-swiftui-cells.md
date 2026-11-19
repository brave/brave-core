---
name: brave/ios-hosting-configuration-for-swiftui-cells
description:
  'SwiftUI content in a UITableViewCell or UICollectionViewCell is set through
  UIHostingConfiguration, not an embedded UIHostingController.'
rubric: design
severity: should_fix
---

**Rule.** To show a SwiftUI view in a table or collection view cell, assign
`cell.contentConfiguration = UIHostingConfiguration { ... }`. Do not create a
`UIHostingController` per cell and add its view to the cell's `contentView`.

**Why.** `UIHostingConfiguration` is the cell configuration API made for this:
it sizes the cell, handles reuse, and needs no child view controller
containment, which a manually embedded hosting controller has to get right
itself.

## Examples

Bad:

```swift
let host = UIHostingController(rootView: MySwiftUIView(item: item))
cell.contentView.addSubview(host.view)
```

Good:

```swift
cell.contentConfiguration = UIHostingConfiguration {
  MySwiftUIView(item: item)
}
```
