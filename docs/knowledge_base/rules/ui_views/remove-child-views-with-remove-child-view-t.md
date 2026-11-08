---
name: brave/remove-child-views-with-remove-child-view-t
description:
  A child view removed from the hierarchy to be destroyed is removed with
  View::RemoveChildViewT(), whose std::unique_ptr owns it, not with
  RemoveChildView(), which does not delete it.
rubric: safety
severity: should_fix
---

**Rule.** To take a child out of a view hierarchy and destroy it, call
`parent->RemoveChildViewT(child)` and let the returned `std::unique_ptr` delete
it (or keep it while it is needed). Do not call `RemoveChildView(View*)` for
this: it does not delete the child, so the view either leaks or has to be
deleted by hand.

**Why.** `RemoveChildViewT()` hands ownership back explicitly, so destruction is
explicit too; `RemoveChildView()` leaves an unowned view behind, with the risk
of a leak or a double free.

## Examples

Bad:

```cpp
parent->RemoveChildView(some_child);  // some_child is not deleted
```

Good:

```cpp
std::unique_ptr<views::View> owned = parent->RemoveChildViewT(some_child);
// owned goes out of scope and deletes the view
```
