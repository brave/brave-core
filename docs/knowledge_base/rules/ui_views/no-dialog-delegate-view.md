---
name: brave/no-dialog-delegate-view
description:
  New modal dialogs do not inherit from the deprecated
  views::DialogDelegateView; they inherit views::DialogDelegate alone and set a
  separate contents view.
rubric: design
severity: should_fix
---

**Rule.** Do NOT inherit from `views::DialogDelegateView`. For a non-bubble
modal dialog, inherit from `views::DialogDelegate` alone, configure its title
and buttons in the constructor, give it a separate contents view with
`SetContentsView()`, and show it with
`views::DialogDelegate::CreateDialogWidget()`.

**Why.** Chromium deprecates `DialogDelegateView`. Combining the delegate and
the view in one class makes ownership and lifetimes harder to reason about, and
encourages mixing UI layout with business logic.

## Examples

Bad:

```cpp
class MyDialog : public views::DialogDelegateView { ... };
```

Good:

```cpp
class MyDialog : public views::DialogDelegate {
 public:
  MyDialog() {
    SetTitle(u"My Dialog");
    SetButtons(static_cast<int>(ui::mojom::DialogButton::kOk) |
               static_cast<int>(ui::mojom::DialogButton::kCancel));
    SetContentsView(std::make_unique<MyContentsView>());
  }
};

views::Widget* widget = views::DialogDelegate::CreateDialogWidget(
    new MyDialog(), /*context=*/nullptr, /*parent=*/nullptr);
widget->Show();
```
