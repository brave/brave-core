---
name: brave/no-remove-observer-in-tab-strip-model-observers
description:
  A TabStripModelObserver subclass never calls
  TabStripModel::RemoveObserver(this) from its destructor, Shutdown() or
  OnTabStripModelDestroyed(); the base class already unregisters it.
rubric: correctness
severity: should_fix
---

**Rule.** In a class inheriting `TabStripModelObserver`, do NOT call
`TabStripModel::RemoveObserver(this)` in the destructor, in `Shutdown()`, or in
`OnTabStripModelDestroyed()`. Clear only the class's own pointers and state
there (for example `tab_strip_model_ = nullptr`), and leave the observer list to
the base class.

**Why.** `~TabStripModelObserver` calls `RemoveObserver(this)` on every model in
`observed_models_`, undoing each `AddObserver`, so derived code duplicates it. A
model being destroyed notifies its observers through `ModelDestroyed`, which
removes the observer before `OnTabStripModelDestroyed()` runs, so a call there
is redundant too.

## Examples

Bad:

```cpp
MyView::~MyView() {
  if (tab_strip_model_) {
    tab_strip_model_->RemoveObserver(this);
  }
}

void MyView::OnTabStripModelDestroyed(TabStripModel* model) {
  model->RemoveObserver(this);
}
```

Good:

```cpp
MyView::~MyView() {
  tab_strip_model_ = nullptr;
}

void MyView::OnTabStripModelDestroyed(TabStripModel* model) {
  tab_strip_model_ = nullptr;
}
```
