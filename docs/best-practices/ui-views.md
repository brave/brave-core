# UI/Views Best Practices

<!-- See also: coding-standards-memory.md -->

<a id="UV-001"></a>

## ✅ Use unique_ptr for View Child Add/Remove

**Prefer the `unique_ptr` overload of `View::AddChildView()` and use
`View::RemoveChildViewT()` when you need to remove a child from the hierarchy
and then destroy it.** `RemoveChildView(View*)` does not delete the child;
`RemoveChildViewT()` returns a `unique_ptr` so ownership and destruction are
explicit.

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

**Do not call `TabStripModel::RemoveObserver(this)` from your subclass
destructor, `Shutdown()`, or `OnTabStripModelDestroyed`.**
`~TabStripModelObserver` copies every `TabStripModel*` in `observed_models_` and
calls `RemoveObserver(this)` on each. Registration via `AddObserver` /
`StartedObserving` is undone by that base destructor, so derived code must not
duplicate it.

When a `TabStripModel` is destroyed, it also notifies observers through
`ModelDestroyed`, which removes the observer before `OnTabStripModelDestroyed`;
explicit `RemoveObserver` there is redundant as well.

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

<a id="UV-003"></a>

## ✅ Prefer `views::AsViewClass` and `views::IsViewClass` Over `static_cast<>` for View Downcasting

**Use `views::AsViewClass<T>()` and `views::IsViewClass<T>()` instead of
`static_cast<T*>()` when downcasting `views::View` pointers.**

Since Chromium disables RTTI, `dynamic_cast` is unavailable. `AsViewClass` and
`IsViewClass` fill that role for the views hierarchy: they walk the metadata
chain registered via `METADATA_HEADER` and catch — at compile time — target
classes that have omitted `METADATA_HEADER` or left a metadata path
unoverridden. At runtime, `AsViewClass` returns `nullptr` on a type mismatch,
including for partially constructed views that have not yet installed their own
class metadata. `static_cast` does none of this and silently produces a pointer
whose dereference is undefined behaviour on a type mismatch.

```cpp
// ❌ WRONG - no type check; UB on dereference if type assumption is wrong.
//            static_cast also cannot detect a partially constructed object —
//            e.g. when a Brave-specific subclass overrides a Chromium view and
//            the object is accessed before the subclass metadata is installed,
//            UBSan will not catch the invalid access.
auto* my_view = static_cast<MyCustomView*>(some_view);
my_view->DoSomething();

// ✅ CORRECT - metadata chain walk returns nullptr for a partially constructed
//              object or any type mismatch, making it safe under UBSan
auto* my_view = views::AsViewClass<MyCustomView>(some_view);
if (!my_view) {
  return;
}
my_view->DoSomething();

// ✅ CORRECT - type predicate
if (views::IsViewClass<MyCustomView>(some_view)) {
  // ...
}
```

---

<a id="UV-004"></a>

## ❌ Avoid `BubbleDialogDelegateView`

**Do not inherit from `BubbleDialogDelegateView`; it is deprecated by
Chromium.** Combining the delegate and view into one class makes ownership and
lifetimes harder to reason about, and encourages mixing UI layout with business
logic.

For bubble dialogs, build a `ui::DialogModel` and return a
`std::unique_ptr<views::BubbleDialogModelHost>` from a factory function. The
caller stores the host and creates a client-owned widget via
`BubbleDialogDelegate::CreateBubble()`, passing a close callback to clean up
when the widget is destroyed.

```cpp
// ❌ WRONG
class MyBubble : public views::BubbleDialogDelegateView { ... };

// ✅ CORRECT - factory returns the host; caller owns both host and widget

// Factory (e.g. in a separate file):
std::unique_ptr<views::BubbleDialogModelHost> ShowMyBubble(
    views::View* anchor_view) {
  auto model = ui::DialogModel::Builder()
      .SetTitle(u"My Bubble")
      .AddOkButton(base::DoNothing())
      .Build();
  return std::make_unique<views::BubbleDialogModelHost>(
      std::move(model), anchor_view, views::BubbleBorder::TOP_RIGHT);
}

// Caller:
bubble_host_ = ShowMyBubble(anchor_view);
bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
    bubble_host_.get(),
    base::BindOnce(&MyClass::OnBubbleClosing,
                   weak_ptr_factory_.GetWeakPtr()));
bubble_widget_->Show();
```

---

<a id="UV-005"></a>

## ❌ Avoid `DialogDelegateView`

**Do not inherit from `DialogDelegateView`; it is deprecated by Chromium.**
Combining the delegate and view into one class makes ownership and lifetimes
harder to reason about, and encourages mixing UI layout with business logic.

For non-bubble modal dialogs, inherit from `DialogDelegate` alone, configure
buttons and title in the constructor, call `SetContentsView()` with a separate
`View`, and show via `DialogDelegate::CreateDialogWidget()`.

```cpp
// ❌ WRONG
class MyDialog : public views::DialogDelegateView { ... };

// ✅ CORRECT - separate delegate + view
class MyDialog : public views::DialogDelegate {
 public:
  MyDialog() {
    SetTitle(u"My Dialog");
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk) |
               static_cast<int>(ui::mojom::DialogButton::kCancel));
    SetContentsView(std::make_unique<MyContentsView>());
  }
};

// Show the dialog:
views::Widget* widget = views::DialogDelegate::CreateDialogWidget(
    new MyDialog(), /*context=*/nullptr, /*parent=*/nullptr);
widget->Show();
```

---
