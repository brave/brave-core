/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_items_scroll_view.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_service_factory.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_drag_context.h"
#include "brave/browser/ui/views/sidebar/sidebar_item_view.h"
#include "brave/browser/ui/views/sidebar/sidebar_items_contents_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "brave/grit/brave_generated_resources.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_format_type.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/layer_tree_owner.h"
#include "ui/events/event.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/focus_ring.h"

namespace {

constexpr char kSidebarItemDragType[] = "brave/sidebar-item";
constexpr int kArrowHeight = 24;

class SidebarItemsArrowView : public views::ImageButton {
 public:
  METADATA_HEADER(SidebarItemsArrowView);
  explicit SidebarItemsArrowView(const std::u16string& accessible_name) {
    SetImageHorizontalAlignment(views::ImageButton::ALIGN_CENTER);
    SetImageVerticalAlignment(views::ImageButton::ALIGN_MIDDLE);
    DCHECK(GetInstallFocusRingOnFocus());
    views::FocusRing::Get(this)->SetColorId(gfx::kBraveBlurple300);
    SetAccessibleName(accessible_name);
    SetPaintToLayer();
  }

  ~SidebarItemsArrowView() override = default;

  SidebarItemsArrowView(const SidebarItemsArrowView&) = delete;
  SidebarItemsArrowView& operator=(const SidebarItemsArrowView&) = delete;

  gfx::Size CalculatePreferredSize() const override {
    return {
        SidebarButtonView::kSidebarButtonSize + SidebarButtonView::kMargin * 2,
        kArrowHeight};
  }

  void OnPaintBackground(gfx::Canvas* canvas) override {
    if (const ui::ColorProvider* color_provider = GetColorProvider()) {
      const SkColor background_color = color_provider->GetColor(kColorToolbar);
      gfx::Rect bounds = GetContentsBounds();
      canvas->FillRect(bounds, background_color);

      // Draw additional rounded rect over background for hover effect.
      if (GetState() == STATE_HOVERED) {
        const SkColor hovered_bg_color =
            color_provider->GetColor(kColorSidebarArrowBackgroundHovered);
        cc::PaintFlags flags;
        flags.setColor(hovered_bg_color);
        flags.setStyle(cc::PaintFlags::kFill_Style);
        // Use smaller area for hover rounded rect.
        constexpr int kInset = 2, kRadius = 34;
        bounds.Inset(gfx::Insets::VH(kInset, 0));
        canvas->DrawRoundRect(bounds, kRadius, flags);
      }
    }
  }
};

BEGIN_METADATA(SidebarItemsArrowView, views::ImageButton)
END_METADATA

}  // namespace

SidebarItemsScrollView::SidebarItemsScrollView(BraveBrowser* browser)
    : browser_(browser),
      drag_context_(std::make_unique<SidebarItemDragContext>()),
      scroll_animator_for_item_(std::make_unique<views::BoundsAnimator>(this)),
      scroll_animator_for_smooth_(
          std::make_unique<views::BoundsAnimator>(this)) {
  model_observed_.Observe(browser->sidebar_controller()->model());
  bounds_animator_observed_.AddObservation(scroll_animator_for_item_.get());
  bounds_animator_observed_.AddObservation(scroll_animator_for_smooth_.get());
  contents_view_ =
      AddChildView(std::make_unique<SidebarItemsContentsView>(browser_, this));
  up_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ITEMS_SCROLL_UP_BUTTON_ACCESSIBLE_NAME)));
  up_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), up_arrow_));
  down_arrow_ = AddChildView(std::make_unique<SidebarItemsArrowView>(
      brave_l10n::GetLocalizedResourceUTF16String(
          IDS_SIDEBAR_ITEMS_SCROLL_DOWN_BUTTON_ACCESSIBLE_NAME)));
  down_arrow_->SetCallback(
      base::BindRepeating(&SidebarItemsScrollView::OnButtonPressed,
                          base::Unretained(this), down_arrow_));

  // To prevent drawing each item's inkdrop layer.
  SetPaintToLayer();
  layer()->SetMasksToBounds(true);
  layer()->SetFillsBoundsOpaquely(false);
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
  if (!event->IsMouseWheelEvent()) {
    return;
  }

  if (!IsScrollable()) {
    return;
  }

  const int y_offset = event->AsMouseWheelEvent()->y_offset();
  if (y_offset == 0) {
    return;
  }

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
  if (scroll_animator_for_item_.get() == animator &&
      lastly_added_item_index_.has_value()) {
    contents_view_->ShowItemAddedFeedbackBubble(*lastly_added_item_index_);
    lastly_added_item_index_ = std::nullopt;
  }
  UpdateArrowViewsEnabledState();
}

void SidebarItemsScrollView::OnItemAdded(const sidebar::SidebarItem& item,
                                         size_t index,
                                         bool user_gesture) {
  contents_view_->OnItemAdded(item, index, user_gesture);

  // Calculate and set this view's bounds to determine whether this view is
  // scroll mode or not.
  parent()->Layout();

  // Only show item added feedback bubble on active browser window if this new
  // item is explicitely by user gesture.
  if (user_gesture && browser_ == BrowserList::GetInstance()->GetLastActive()) {
    // If added item is not visible because of narrow height, we should scroll
    // to make it visible.
    if (NeedScrollForItemAt(index)) {
      lastly_added_item_index_ = index;
      scroll_animator_for_item_->AnimateViewTo(
          contents_view_, GetTargetScrollContentsViewRectForItemAt(index));
    } else {
      contents_view_->ShowItemAddedFeedbackBubble(index);
    }
  }
}

void SidebarItemsScrollView::OnItemMoved(const sidebar::SidebarItem& item,
                                         size_t from,
                                         size_t to) {
  contents_view_->OnItemMoved(item, from, to);
}

void SidebarItemsScrollView::OnItemRemoved(size_t index) {
  contents_view_->OnItemRemoved(index);
}

void SidebarItemsScrollView::OnActiveIndexChanged(
    std::optional<size_t> old_index,
    std::optional<size_t> new_index) {
  // If activated item is not visible, scroll to show it.
  if (new_index && NeedScrollForItemAt(*new_index)) {
    scroll_animator_for_item_->AnimateViewTo(
        contents_view_, GetTargetScrollContentsViewRectForItemAt(*new_index));
  }
  contents_view_->OnActiveIndexChanged(old_index, new_index);
}

void SidebarItemsScrollView::OnItemUpdated(
    const sidebar::SidebarItem& item,
    const sidebar::SidebarItemUpdate& update) {
  contents_view_->UpdateItem(item, update);
}

void SidebarItemsScrollView::OnFaviconUpdatedForItem(
    const sidebar::SidebarItem& item,
    const gfx::ImageSkia& image) {
  contents_view_->SetImageForItem(item, image);
}

void SidebarItemsScrollView::UpdateArrowViewsTheme() {
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    const SkColor arrow_normal =
        color_provider->GetColor(kColorSidebarArrowNormal);
    const SkColor arrow_disabled =
        color_provider->GetColor(kColorSidebarArrowDisabled);

    up_arrow_->SetImageModel(
        views::Button::STATE_NORMAL,
        ui::ImageModel::FromVectorIcon(kSidebarItemsUpArrowIcon, arrow_normal));
    up_arrow_->SetImageModel(views::Button::STATE_DISABLED,
                             ui::ImageModel::FromVectorIcon(
                                 kSidebarItemsUpArrowIcon, arrow_disabled));
    down_arrow_->SetImageModel(views::Button::STATE_NORMAL,
                               ui::ImageModel::FromVectorIcon(
                                   kSidebarItemsDownArrowIcon, arrow_normal));
    down_arrow_->SetImageModel(views::Button::STATE_DISABLED,
                               ui::ImageModel::FromVectorIcon(
                                   kSidebarItemsDownArrowIcon, arrow_disabled));
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
  if (bounds().IsEmpty() || GetPreferredSize().IsEmpty()) {
    return false;
  }

  return bounds().height() < GetPreferredSize().height();
}

void SidebarItemsScrollView::OnButtonPressed(views::View* view) {
  const int scroll_offset =
      SidebarButtonView::kSidebarButtonSize + SidebarButtonView::kMargin;
  if (view == up_arrow_) {
    ScrollContentsViewBy(scroll_offset, true);
  }

  if (view == down_arrow_) {
    ScrollContentsViewBy(-scroll_offset, true);
  }

  UpdateArrowViewsEnabledState();
}

void SidebarItemsScrollView::ScrollContentsViewBy(int offset, bool animate) {
  if (offset == 0) {
    return;
  }

  // If scroll goes up, it should not go further if the origin of contents view
  // is same with bottom_right of up arrow.
  if (offset > 0) {
    const gfx::Rect up_arrow_bounds = up_arrow_->bounds();
    // If contents view already stick to up_arrow bottom, just return.
    if (contents_view_->origin() == up_arrow_bounds.bottom_left()) {
      return;
    }

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
    if (contents_view_->bounds().bottom_left() == down_arrow_bounds.origin()) {
      return;
    }

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

bool SidebarItemsScrollView::NeedScrollForItemAt(size_t index) const {
  if (!IsScrollable()) {
    return false;
  }

  views::View* item_view = contents_view_->children()[index];
  auto item_view_bounds_per_scroll_view = item_view->GetLocalBounds();
  item_view_bounds_per_scroll_view = views::View::ConvertRectToTarget(
      item_view, this, item_view_bounds_per_scroll_view);

  auto scroll_view_bounds = GetContentsBounds();
  scroll_view_bounds.Inset(gfx::Insets::VH(kArrowHeight, 0));

  // Need scroll if item is not fully included in scroll view.
  return !scroll_view_bounds.Contains(item_view_bounds_per_scroll_view);
}

gfx::Rect SidebarItemsScrollView::GetTargetScrollContentsViewRectForItemAt(
    size_t index) const {
  DCHECK(NeedScrollForItemAt(index));

  views::View* item_view = contents_view_->children()[index];
  auto item_view_bounds_per_scroll_view = item_view->GetLocalBounds();
  item_view_bounds_per_scroll_view = views::View::ConvertRectToTarget(
      item_view, this, item_view_bounds_per_scroll_view);

  const bool scroll_up = item_view_bounds_per_scroll_view.bottom() >
                         (GetContentsBounds().bottom() - kArrowHeight);
  auto item_view_bounds = item_view->GetLocalBounds();
  item_view_bounds = views::View::ConvertRectToTarget(item_view, contents_view_,
                                                      item_view_bounds);
  gfx::Rect target_bounds = contents_view_->bounds();

  if (scroll_up) {
    // Scroll to make this item as a last visible item.
    target_bounds.set_origin({contents_view_->origin().x(),
                              GetContentsBounds().height() -
                                  item_view_bounds.bottom() - kArrowHeight});
  } else {
    // Scroll to make this item as a first visible item.
    target_bounds.set_origin(
        {contents_view_->origin().x(), -item_view_bounds.y() + kArrowHeight});
  }

  return target_bounds;
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
  if (!HitTestPoint(position)) {
    return false;
  }

  // If this is not scrollable, this scroll view shows all contents view.
  if (!IsScrollable()) {
    return true;
  }

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
  if (!drag_context_->source()) {
    return false;
  }

  return data.HasCustomFormat(
      ui::ClipboardFormatType::GetType(kSidebarItemDragType));
}

void SidebarItemsScrollView::OnDragExited() {
  ClearDragIndicator();
}

void SidebarItemsScrollView::ClearDragIndicator() {
  contents_view_->ClearDragIndicator();
  drag_context_->set_drag_indicator_index(std::nullopt);
}

int SidebarItemsScrollView::OnDragUpdated(const ui::DropTargetEvent& event) {
  auto ret = ui::DragDropTypes::DRAG_NONE;

  // This scroll view is the visible area of items contents view.
  // If dragging point is in this scroll view, draw indicator.
  if (IsInVisibleContentsViewBounds(event.location())) {
    gfx::Point screen_position = event.location();
    views::View::ConvertPointToScreen(this, &screen_position);
    views::View* view = drag_context_->source();
    auto target_index =
        contents_view_->DrawDragIndicator(view, screen_position);
    drag_context_->set_drag_indicator_index(target_index);
    ret = ui::DragDropTypes::DRAG_MOVE;
  } else {
    ClearDragIndicator();
  }

  return ret;
}

views::View::DropCallback SidebarItemsScrollView::GetDropCallback(
    const ui::DropTargetEvent& event) {
  return base::BindOnce(&SidebarItemsScrollView::PerformDrop,
                        weak_ptr_.GetWeakPtr());
}

void SidebarItemsScrollView::PerformDrop(
    const ui::DropTargetEvent& event,
    ui::mojom::DragOperation& output_drag_op,
    std::unique_ptr<ui::LayerTreeOwner> drag_image_layer_owner) {
  output_drag_op = ui::mojom::DragOperation::kNone;
  if (drag_context_->ShouldMoveItem()) {
    output_drag_op = ui::mojom::DragOperation::kMove;
    auto* service =
        sidebar::SidebarServiceFactory::GetForProfile(browser_->profile());
    service->MoveItem(*drag_context_->source_index(),
                      drag_context_->GetTargetIndex());
  }

  contents_view_->ClearDragIndicator();
  drag_context_->Reset();
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
  return drag_context_->source_index() != std::nullopt;
}

bool SidebarItemsScrollView::IsBubbleVisible() const {
  return contents_view_->IsBubbleVisible();
}

void SidebarItemsScrollView::Update() {
  contents_view_->Update();
}

BEGIN_METADATA(SidebarItemsScrollView, views::View)
END_METADATA
