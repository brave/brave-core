/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"

#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/views/sidebar/sidebar_button_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "chrome/browser/ui/browser_list.h"
#include "cc/paint/paint_flags.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"

namespace {

class SidebarItemsArrowView : public views::ImageButton {
 public:
  SidebarItemsArrowView() {
    SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
    SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
  }

  ~SidebarItemsArrowView() override = default;

  SidebarItemsArrowView(const SidebarItemsArrowView&) = delete;
  SidebarItemsArrowView& operator=(const SidebarItemsArrowView&) = delete;

  gfx::Size CalculatePreferredSize() const override { return {42, 24}; }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
      const SkColor background_color = theme_provider->GetColor(
          BraveThemeProperties::COLOR_SIDEBAR_BACKGROUND);
      gfx::Rect bounds = GetContentsBounds();
      canvas->FillRect(bounds, background_color);

      // Draw additional rounded rect over background for hover effect.
      if (GetState() == STATE_HOVERED) {
        const SkColor hovered_bg_color = theme_provider->GetColor(
            BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED);
        cc::PaintFlags flags;
        flags.setColor(hovered_bg_color);
        flags.setStyle(cc::PaintFlags::kFill_Style);
        // Use smaller area for hover rounded rect.
        constexpr int kInset = 2, kRadius = 34;
        bounds.Inset(kInset, 0);
        canvas->DrawRoundRect(bounds, kRadius, flags);
      }
    }
  }
};

}  // namespace

SidebarItemsScrollView::SidebarItemsScrollView(BraveBrowser* browser)
    : browser_(browser),
      scroll_animator_for_new_item_(
          std::make_unique<views::BoundsAnimator>(this)),
      scroll_animator_for_smooth_(
          std::make_unique<views::BoundsAnimator>(this)) {
  model_observed_.Add(browser->sidebar_controller()->model());
  bounds_animator_observed_.Add(scroll_animator_for_new_item_.get());
  bounds_animator_observed_.Add(scroll_animator_for_smooth_.get());
  contents_view_ =
      AddChildView(std::make_unique<SidebarItemsContentsView>(browser_));
  up_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>());
  up_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), up_arrow_));
  down_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>());
  down_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), down_arrow_));
}

SidebarItemsScrollView::~SidebarItemsScrollView() = default;

void SidebarItemsScrollView::Layout() {
  // |contents_view_| always has it's preferred size. and this scroll view only
  // shows some parts of it if scroll view can't get enough rect.
  contents_view_->SizeToPreferredSize();

  const bool show_arrow = IsScrollable();
  const bool arrow_was_not_shown = !up_arrow_->GetVisible();
  up_arrow_->SetVisible(show_arrow);
  down_arrow_->SetVisible(show_arrow);

  const gfx::Rect bounds = GetContentsBounds();
  const int arrow_height = up_arrow_->GetPreferredSize().height();
  // Locate arrows.
  if (show_arrow) {
    up_arrow_->SizeToPreferredSize();
    up_arrow_->SetPosition(bounds.origin());
    down_arrow_->SizeToPreferredSize();
    down_arrow_->SetPosition({bounds.x(), bounds.bottom() - arrow_height});
  }

  if (show_arrow) {
    // Attach contents view to up arrow view when overflow mode is started.
    if (arrow_was_not_shown) {
      contents_view_->SetPosition(up_arrow_->bounds().bottom_left());
      UpdateArrowViewsEnabledState();
      return;
    }

    // Pull contents view when scroll view is getting longer.
    int dist = down_arrow_->bounds().y() - contents_view_->bounds().bottom();
    if (dist > 0) {
      contents_view_->SetPosition(
          {contents_view_->x(), contents_view_->y() + dist});
    }

    UpdateArrowViewsEnabledState();
  } else {
    // Scroll view has enough space for contents view.
    contents_view_->SetPosition(bounds.origin());
  }
}

void SidebarItemsScrollView::OnMouseEvent(ui::MouseEvent* event) {
  if (!event->IsMouseWheelEvent())
    return;

  if (!IsScrollable())
    return;

  const int y_offset = event->AsMouseWheelEvent()->y_offset();
  if (y_offset == 0)
    return;

  ScrollContentsViewBy(y_offset, false);
  UpdateArrowViewsEnabledState();
}

gfx::Size SidebarItemsScrollView::CalculatePreferredSize() const {
  DCHECK(contents_view_);
  return contents_view_->GetPreferredSize() + GetInsets().size();
}

void SidebarItemsScrollView::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateArrowViewsTheme();
}

void SidebarItemsScrollView::OnBoundsAnimatorProgressed(
    views::BoundsAnimator* animator) {
}

void SidebarItemsScrollView::OnBoundsAnimatorDone(
    views::BoundsAnimator* animator) {
  if (scroll_animator_for_new_item_.get() == animator) {
    contents_view_->ShowItemAddedFeedbackBubble();
    UpdateArrowViewsEnabledState();
  }
}

void SidebarItemsScrollView::OnItemAdded(const sidebar::SidebarItem& item,
                                         int index,
                                         bool user_gesture) {
  contents_view_->OnItemAdded(item, index, user_gesture);

  // Calculate and set this view's bounds to determine whether this view is
  // scroll mode or not.
  parent()->Layout();

  // Only show item added feedback bubble on active browser window if this new
  // item is explicitely by user gesture.
  if (user_gesture && browser_ == BrowserList::GetInstance()->GetLastActive()) {
    // If scrollable, scroll to bottom with animation before showing feedback.
    if (IsScrollable()) {
      scroll_animator_for_new_item_->AnimateViewTo(
          contents_view_, GetTargetScrollContentsViewRectTo(false));
    } else {
      contents_view_->ShowItemAddedFeedbackBubble();
    }
  }
}

void SidebarItemsScrollView::OnItemRemoved(int index) {
  contents_view_->OnItemRemoved(index);
}

void SidebarItemsScrollView::OnActiveIndexChanged(
    int old_index, int new_index) {
  contents_view_->OnActiveIndexChanged(old_index, new_index);
}
void SidebarItemsScrollView::OnFaviconUpdatedForItem(
    const sidebar::SidebarItem& item, const gfx::ImageSkia& image) {
  contents_view_->SetImageForItem(item, image);
}

void SidebarItemsScrollView::UpdateArrowViewsTheme() {
  if (const ui::ThemeProvider* theme_provider = GetThemeProvider()) {
    const SkColor arrow_normal = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL);
    const SkColor arrow_disabled = theme_provider->GetColor(
        BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED);

    up_arrow_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarItemsUpArrowIcon, arrow_normal));
    up_arrow_->SetImage(
        views::Button::STATE_DISABLED,
        gfx::CreateVectorIcon(kSidebarItemsUpArrowIcon, arrow_disabled));
    down_arrow_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(kSidebarItemsDownArrowIcon, arrow_normal));
    down_arrow_->SetImage(
        views::Button::STATE_DISABLED,
        gfx::CreateVectorIcon(kSidebarItemsDownArrowIcon, arrow_disabled));
  }
}

void SidebarItemsScrollView::UpdateArrowViewsEnabledState() {
  DCHECK(up_arrow_->GetVisible() && down_arrow_->GetVisible());
  const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
  const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
  up_arrow_->SetEnabled(contents_view_->origin() !=
                        up_arrow_bounds.bottom_left());
  down_arrow_->SetEnabled(contents_view_->bounds().bottom_left() !=
                          down_arrow_bounds.origin());
}

bool SidebarItemsScrollView::IsScrollable() const {
  if (bounds().IsEmpty() || GetPreferredSize().IsEmpty())
    return false;

  return bounds().height() < GetPreferredSize().height();
}

void SidebarItemsScrollView::OnButtonPressed(views::View* view) {
  const int scroll_offset = SidebarButtonView::kSidebarButtonSize;
  if (view == up_arrow_)
    ScrollContentsViewBy(scroll_offset, true);

  if (view == down_arrow_)
    ScrollContentsViewBy(-scroll_offset, true);

  UpdateArrowViewsEnabledState();
}

void SidebarItemsScrollView::ScrollContentsViewBy(int offset, bool animate) {
  if (offset == 0)
    return;

  // If scroll goes up, it should not go further if the origin of contents view
  // is same with bottom_right of up arrow.
  if (offset > 0) {
    const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
    // If contents view already stick to up_arrow bottom, just return.
    if (contents_view_->origin() == up_arrow_bounds.bottom_left())
      return;

    // If contents view top meets or exceeds the up arrow bottom, attach it to
    // up arrow bottom.
    if ((contents_view_->origin().y() + offset) >= up_arrow_bounds.bottom()) {
      if (animate) {
        scroll_animator_for_smooth_->AnimateViewTo(
            contents_view_, GetTargetScrollContentsViewRectTo(true));
      } else {
        contents_view_->SetBoundsRect(GetTargetScrollContentsViewRectTo(true));
      }
      return;
    }
  }

  // If scroll goes down, it should not go further if the bottom left of
  // contents view is same with origin of down arrow.
  if (offset < 0) {
    const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
    // If contents view already stick to down_arrow top, just return.
    if (contents_view_->bounds().bottom_left() == down_arrow_bounds.origin())
      return;

    // If contents view bottom meets or exceeds the down arrow top, attach it to
    // down arrow top.
    if ((contents_view_->bounds().bottom() + offset) <= down_arrow_bounds.y()) {
      if (animate) {
        scroll_animator_for_smooth_->AnimateViewTo(
            contents_view_, GetTargetScrollContentsViewRectTo(false));
      } else {
        contents_view_->SetBoundsRect(GetTargetScrollContentsViewRectTo(false));
      }
      return;
    }
  }

  // Move contents view vertically for |offset|.
  gfx::Rect target_bounds = contents_view_->bounds();
  target_bounds.set_origin({contents_view_->origin().x(),
                            contents_view_->origin().y() + offset});
  if (animate) {
    scroll_animator_for_smooth_->AnimateViewTo(contents_view_, target_bounds);
  } else {
    contents_view_->SetPosition(target_bounds.origin());
  }
}

gfx::Rect SidebarItemsScrollView::GetTargetScrollContentsViewRectTo(bool top) {
  gfx::Rect target_bounds;
  const gfx::Rect contents_bounds = contents_view_->bounds();
  target_bounds.set_x(contents_bounds.x());
  if (top) {
    const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
    target_bounds.set_y(up_arrow_bounds.bottom());
  } else {
    const gfx::Rect down_arrow_bounds = down_arrow_->bounds();
    target_bounds.set_y(down_arrow_bounds.y() - contents_view_->height());
  }

  target_bounds.set_size(contents_bounds.size());
  return target_bounds;
}
