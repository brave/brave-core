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
caller transfers host ownership to the widget via `release()` and holds the
widget itself. A close callback posted with `DeleteSoon` destroys the
client-owned widget asynchronously.

```cpp
// ❌ WRONG
class MyBubble : public views::BubbleDialogDelegateView { ... };

// ✅ CORRECT

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

// Caller — host ownership is transferred to the widget:
std::unique_ptr<views::BubbleDialogModelHost> host = ShowMyBubble(anchor_view);
bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
    host.release(),
    base::BindOnce(&MyClass::OnBubbleClosing,
                   weak_ptr_factory_.GetWeakPtr()));
bubble_widget_->Show();

// Close callback — use DeleteSoon to destroy the client-owned widget:
void MyClass::OnBubbleClosing(views::Widget::ClosedReason reason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, bubble_widget_.release());
}
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

<a id="UV-006"></a>

## ❌ Don't Use `ImageFetcher` for Off-the-Record or Tor Profiles

**`ImageFetcherService` sends its requests through the browser-wide
`SystemNetworkContextManager` network context, which is not bound to any
profile.** For an off-the-record profile the request leaves the profile's
network partition, and in a Tor window it bypasses the Tor proxy and goes
straight to the destination host (leaking the user's IP and the visited site).
Only use `ImageFetcher` when the profile is a regular profile; skip the fetch
and fall back to a default (e.g. first letter of the hostname) otherwise.

```cpp
// ❌ WRONG - fetches through the system network context for any profile
void SidebarModel::FetchFaviconFromNetwork(const SidebarItem& item) {
  image_fetcher_->FetchImage(item.url, ...);
}

// ✅ CORRECT - skip the fetch for off-the-record (incl. Tor) profiles
void SidebarModel::FetchFaviconFromNetwork(const SidebarItem& item) {
  if (profile_->IsOffTheRecord()) {
    return;  // Caller falls back to a default favicon.
  }
  image_fetcher_->FetchImage(item.url, ...);
}
```

Any new use of `ImageFetcher` must be checked for this (e.g. favicon fetching
for default search engine promotions). When a request must be profile-scoped,
use a `SimpleURLLoader` with the profile's `URLLoaderFactory` instead.

---

<a id="UV-007"></a>

## ✅ Mark Clipboard Writes from Off-the-Record Profiles as Off-the-Record

**When browser UI code writes content derived from an off-the-record profile
(Incognito, Private, Tor) to the clipboard, call
`ScopedClipboardWriter::MarkAsOffTheRecord()`.** It sets
`Clipboard::kNoLocalClipboardHistory | Clipboard::kNoCloudClipboard`, so
OS-level clipboard history (e.g. Windows Win+V) and cloud clipboard sync to
other devices do not retain the data. Without it, private-window content (such
as screenshots) leaks outside the private session. Tor profiles are
off-the-record, so `IsOffTheRecord()` covers them without a separate check.

```cpp
// ❌ WRONG - private-window content may land in clipboard history / cloud sync
ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
writer.WriteImage(bitmap);

// ✅ CORRECT - mirrors content::ClipboardHostImpl and OmniboxViewViews
ui::ScopedClipboardWriter writer(ui::ClipboardBuffer::kCopyPaste);
writer.WriteImage(bitmap);
if (profile->IsOffTheRecord()) {
  writer.MarkAsOffTheRecord();
}
```

Cover both branches in unit tests with a `ui::TestClipboard` subclass that
records the `privacy_types` passed to
`WritePortableAndPlatformRepresentations()` (including a regular-profile
negative case).

---
