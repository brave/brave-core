# UI/Views Best Practices

<!-- See also: coding-standards-memory.md -->

<a id="UV-001"></a>

## ✅ Use unique_ptr for View Child Add/Remove

**Prefer the `unique_ptr` overload of `View::AddChildView()` and use `View::RemoveChildViewT()` when you need to remove a child from the hierarchy and then destroy it.** `RemoveChildView(View*)` does not delete the child; `RemoveChildViewT()` returns a `unique_ptr` so ownership and destruction are explicit.

```cpp
// ❌ WRONG - raw pointer add; remove does not transfer ownership
AddChildView(new views::Label(u"Hello"));
parent->RemoveChildView(some_child);  // some_child is not deleted; leak or double-free risk

// ✅ CORRECT - unique_ptr add and remove
auto* label = AddChildView(std::make_unique<views::Label>(u"Hello"));
// When removing and destroying:
std::unique_ptr<views::View> owned = parent->RemoveChildViewT(some_child);
// owned goes out of scope and deletes the view
```

---

