---
name: brave/no-bubble-dialog-delegate-view
description:
  New bubbles do not inherit from the deprecated
  views::BubbleDialogDelegateView; they build a ui::DialogModel shown by a
  views::BubbleDialogModelHost.
rubric: design
severity: should_fix
---

**Rule.** Do NOT inherit from `views::BubbleDialogDelegateView`. For a bubble
dialog, build a `ui::DialogModel` and return a
`std::unique_ptr<views::BubbleDialogModelHost>` from a factory function. The
caller passes the host to `views::BubbleDialogDelegate::CreateBubble()` with
`release()`, transferring its ownership to the widget, holds the widget itself,
and destroys that client-owned widget asynchronously with `DeleteSoon` from the
close callback.

**Why.** Chromium deprecates `BubbleDialogDelegateView`. Combining the delegate
and the view in one class makes ownership and lifetimes harder to reason about,
and encourages mixing UI layout with business logic.

## Examples

Bad:

```cpp
class MyBubble : public views::BubbleDialogDelegateView { ... };
```

Good:

```cpp
std::unique_ptr<views::BubbleDialogModelHost> ShowMyBubble(
    views::View* anchor_view) {
  auto model = ui::DialogModel::Builder()
                   .SetTitle(u"My Bubble")
                   .AddOkButton(base::DoNothing())
                   .Build();
  return std::make_unique<views::BubbleDialogModelHost>(
      std::move(model), anchor_view, views::BubbleBorder::TOP_RIGHT);
}

std::unique_ptr<views::BubbleDialogModelHost> host = ShowMyBubble(anchor_view);
bubble_widget_ = views::BubbleDialogDelegate::CreateBubble(
    host.release(), base::BindOnce(&MyClass::OnBubbleClosing,
                                   weak_ptr_factory_.GetWeakPtr()));
bubble_widget_->Show();

void MyClass::OnBubbleClosing(views::Widget::ClosedReason reason) {
  base::SingleThreadTaskRunner::GetCurrentDefault()->DeleteSoon(
      FROM_HERE, bubble_widget_.release());
}
```
