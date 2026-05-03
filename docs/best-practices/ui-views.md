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

<a id="UV-002"></a>

## ✅ Do not manually call `RemoveObserver` when inheriting `TabStripModelObserver`

**Do not call `TabStripModel::RemoveObserver(this)` from your subclass destructor, `Shutdown()`, or `OnTabStripModelDestroyed`.** `~TabStripModelObserver` copies every `TabStripModel*` in `observed_models_` and calls `RemoveObserver(this)` on each. Registration via `AddObserver` / `StartedObserving` is undone by that base destructor, so derived code must not duplicate it.

When a `TabStripModel` is destroyed, it also notifies observers through `ModelDestroyed`, which removes the observer before `OnTabStripModelDestroyed`; explicit `RemoveObserver` there is redundant as well.

```cpp
// ❌ WRONG - ~TabStripModelObserver already unregisters from every observed model
MyView::~MyView() {
  if (tab_strip_model_) {
    tab_strip_model_->RemoveObserver(this);
  }
}

void MyView::OnTabStripModelDestroyed(TabStripModel* model) {
  model->RemoveObserver(this);  // also redundant
}

// ✅ CORRECT - clear your own pointers or state only; leave observer list to the base class
MyView::~MyView() {
  tab_strip_model_ = nullptr;
}

void MyView::OnTabStripModelDestroyed(TabStripModel* model) {
  tab_strip_model_ = nullptr;
  // ...
}
```

---

