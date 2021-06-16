/* Copyright (c) 2021 The Brave Authors. All rights reserved.
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
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_drag_context.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/browser_list.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/focus_ring.h"

namespace {

constexpr char kSidebarItemDragType[] = "brave/sidebar-item";

class SidebarItemsArrowView : public views::ImageButton {
 public:
  SidebarItemsArrowView() {
    SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
    SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
    DCHECK(GetInstallFocusRingOnFocus());
    focus_ring()->SetColor(gfx::kBraveBlurple300);
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
      drag_context_(std::make_unique<SidebarItemDragContext>()),
      scroll_animator_for_new_item_(
          std::make_unique<views::BoundsAnimator>(this)),
      scroll_animator_for_smooth_(
          std::make_unique<views::BoundsAnimator>(this)) {
  model_observed_.Observe(browser->sidebar_controller()->model());
  bounds_animator_observed_.AddObservation(scroll_animator_for_new_item_.get());
  bounds_animator_observed_.AddObservation(scroll_animator_for_smooth_.get());
  contents_view_ =
      AddChildView(std::make_unique<SidebarItemsContentsView>(browser_, this));
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
    views::BoundsAnimator* animator) {}

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

void SidebarItemsScrollView::OnItemMoved(const sidebar::SidebarItem& item,
                                         int from,
                                         int to) {
  contents_view_->OnItemMoved(item, from, to);
}

void SidebarItemsScrollView::OnItemRemoved(int index) {
  contents_view_->OnItemRemoved(index);
}

void SidebarItemsScrollView::OnActiveIndexChanged(int old_index,
                                                  int new_index) {
  contents_view_->OnActiveIndexChanged(old_index, new_index);
}

void SidebarItemsScrollView::OnFaviconUpdatedForItem(
    const sidebar::SidebarItem& item,
    const gfx::ImageSkia& image) {
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
  target_bounds.set_origin(
      {contents_view_->origin().x(), contents_view_->origin().y() + offset});
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

bool SidebarItemsScrollView::IsInVisibleContentsViewBounds(
    const gfx::Point& position) const {
  if (!HitTestPoint(position))
    return false;

  // If this is not scrollable, this scroll view shows all contents view.
  if (!IsScrollable())
    return true;

  if (up_arrow_->bounds().Contains(position) ||
      down_arrow_->bounds().Contains(position)) {
    return false;
  }

  return true;
}

bool SidebarItemsScrollView::GetDropFormats(
    int* formats,
    std::set<ui::ClipboardFormatType>* format_types) {
  format_types->insert(ui::ClipboardFormatType::GetType(kSidebarItemDragType));
  return true;
}

bool SidebarItemsScrollView::CanDrop(const OSExchangeData& data) {
  // Null means sidebar item drag and drop is not initiated by this view.
  // Don't allow item move from different window.
  if (!drag_context_->source())
    return false;

  return data.HasCustomFormat(
      ui::ClipboardFormatType::GetType(kSidebarItemDragType));
}

int SidebarItemsScrollView::OnDragUpdated(const ui::DropTargetEvent& event) {
  auto ret = ui::DragDropTypes::DRAG_NONE;

  // This scroll view is the visible area of items contents view.
  // If dragging point is in this scroll view, draw indicator.
  if (IsInVisibleContentsViewBounds(event.location())) {
    gfx::Point screen_position = event.location();
    views::View::ConvertPointToScreen(this, &screen_position);
    views::View* view = drag_context_->source();
    const int target_index =
        contents_view_->DrawDragIndicator(view, screen_position);
    drag_context_->set_drag_indicator_index(target_index);
    ret = ui::DragDropTypes::DRAG_MOVE;
  } else {
    contents_view_->ClearDragIndicator();
    drag_context_->set_drag_indicator_index(-1);
  }

  return ret;
}

void SidebarItemsScrollView::OnDragExited() {
  contents_view_->ClearDragIndicator();
  drag_context_->set_drag_indicator_index(-1);
}

ui::mojom::DragOperation SidebarItemsScrollView::OnPerformDrop(
    const ui::DropTargetEvent& event) {
  auto ret = ui::mojom::DragOperation::kNone;

  if (drag_context_->ShouldMoveItem()) {
    auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
    service->MoveItem(drag_context_->source_index(),
                      drag_context_->GetTargetIndex());
    ret = ui::mojom::DragOperation::kMove;
  }

  contents_view_->ClearDragIndicator();
  drag_context_->Reset();
  return ret;
}

void SidebarItemsScrollView::WriteDragDataForView(views::View* sender,
                                                  const gfx::Point& press_pt,
                                                  ui::OSExchangeData* data) {
  SidebarItemView* item_view = static_cast<SidebarItemView*>(sender);
  data->provider().SetDragImage(
      item_view->GetImage(views::Button::STATE_NORMAL),
      press_pt.OffsetFromOrigin());

  data->SetPickledData(ui::ClipboardFormatType::GetType(kSidebarItemDragType),
                       base::Pickle());
}

int SidebarItemsScrollView::GetDragOperationsForView(views::View* sender,
                                                     const gfx::Point& p) {
  return ui::DragDropTypes::DRAG_MOVE;
}

bool SidebarItemsScrollView::CanStartDragForView(views::View* sender,
                                                 const gfx::Point& press_pt,
                                                 const gfx::Point& p) {
  if (SidebarItemDragContext::CanStartDrag(press_pt, p)) {
    drag_context_->Reset();
    drag_context_->set_source(sender);
    drag_context_->set_source_index(contents_view_->GetIndexOf(sender));
    return true;
  }

  return false;
}

bool SidebarItemsScrollView::IsItemReorderingInProgress() const {
  return drag_context_->source_index() != -1;
}

bool SidebarItemsScrollView::IsBubbleVisible() const {
  return contents_view_->IsBubbleVisible();
}
