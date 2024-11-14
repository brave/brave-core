/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/containers/flat_map.h"
#include "base/feature_list.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/grit/theme_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/view_utils.h"

using BrowserRootView::DropIndex::GroupInclusion::kDontIncludeInGroup;
using BrowserRootView::DropIndex::GroupInclusion::kIncludeInGroup;
using BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex;
using BrowserRootView::DropIndex::RelativeToIndex::kReplaceIndex;

BraveTabContainer::BraveTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragContextBase* drag_context,
    TabSlotController& tab_slot_controller,
    views::View* scroll_contents_view)
    : TabContainerImpl(controller,
                       hover_card_controller,
                       drag_context,
                       tab_slot_controller,
                       scroll_contents_view),
      drag_context_(drag_context),
      tab_style_(TabStyle::Get()),
      controller_(controller) {
  auto* browser = tab_slot_controller_->GetBrowser();
  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  if (!tabs::utils::SupportsVerticalTabs(browser)) {
    return;
  }

  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));
  vertical_tabs_floating_mode_enabled_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));
  vertical_tabs_collapsed_.Init(
      brave_tabs::kVerticalTabsCollapsed, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));

  UpdateLayoutOrientation();
}

BraveTabContainer::~BraveTabContainer() {
  // When the last tab is closed and tab strip is being destructed, the
  // animation for the last removed tab could have been scheduled but not
  // finished yet. In this case, stop the animation before checking if all
  // closed tabs were cleaned up from OnTabCloseAnimationCompleted().
  CancelAnimation();
  DCHECK(closing_tabs_.empty()) << "There are dangling closed tabs.";
  DCHECK(!layout_locked_)
      << "The lock returned by LockLayout() shouldn't outlive this object";
}

base::OnceClosure BraveTabContainer::LockLayout() {
  DCHECK(!layout_locked_) << "LockLayout() doesn't allow reentrance";
  layout_locked_ = true;
  return base::BindOnce(&BraveTabContainer::OnUnlockLayout,
                        base::Unretained(this));
}

void BraveTabContainer::AddedToWidget() {
  TabContainerImpl::AddedToWidget();
  auto* browser = tab_slot_controller_->GetBrowser();
  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  if (auto* split_view_data =
          SplitViewBrowserData::FromBrowser(const_cast<Browser*>(browser))) {
    if (!split_view_data_observation_.IsObserving()) {
      split_view_data_observation_.Observe(split_view_data);
    }
  }
}

gfx::Size BraveTabContainer::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return {};
  }

  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::CalculatePreferredSize(available_size);
  }

  const int tab_count = tabs_view_model_.view_size();
  int height = 0;
  if (bounds_animator_.IsAnimating() && tab_count &&
      !static_cast<TabDragContext*>(drag_context_.get())
           ->GetDragController()
           ->IsActive()) {
    // When removing a tab in the middle of tabs, the last tab's current bottom
    // could be greater than ideal bounds bottom.
    height = tabs_view_model_.view_at(tab_count - 1)->bounds().bottom();
  }

  if (!closing_tabs_.empty()) {
    // When closing trailing tabs, the last tab's current bottom could be
    // greater than ideal bounds bottom. Note that closing tabs are not in
    // tabs_view_model_ so we have to check again here.
    for (auto* tab : closing_tabs_) {
      height = std::max(height, tab->bounds().bottom());
    }
  }

  const auto slots_bounds = layout_helper_->CalculateIdealBounds(
      available_width_callback_.is_null() ||
              base::FeatureList::IsEnabled(tabs::kScrollableTabStrip)
          ? std::nullopt
          : std::optional<int>(available_width_callback_.Run()));
  height =
      std::max(height, slots_bounds.empty() ? 0 : slots_bounds.back().bottom());

  if (tab_count) {
    if (Tab* last_tab = tabs_view_model_.view_at(tab_count - 1);
        last_tab->group().has_value() &&
        !controller_->IsGroupCollapsed(*last_tab->group())) {
      height += BraveTabGroupHeader::kPaddingForGroup;
    }

    // Both containers for pinned tabs and unpinned tabs should have margin
    height += tabs::kMarginForVerticalTabContainers;
  }

  return gfx::Size(tab_style_->GetStandardWidth(), height);
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  // Don't shrink vertical tab strip's width
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return;
  }

  TabContainerImpl::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

gfx::Rect BraveTabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::GetTargetBoundsForClosingTab(tab,
                                                          former_model_index);
  }

  gfx::Rect target_bounds = tab->bounds();
  if (tab->data().pinned) {
    target_bounds.set_width(0);
  } else {
    target_bounds.set_y(
        (former_model_index > 0)
            ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom()
            : 0);
    target_bounds.set_height(0);
  }
  return target_bounds;
}

void BraveTabContainer::EnterTabClosingMode(std::optional<int> override_width,
                                            CloseTabSource source) {
  // Don't shrink vertical tab strip's width
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return;
  }

  TabContainerImpl::EnterTabClosingMode(override_width, source);
}

bool BraveTabContainer::ShouldTabBeVisible(const Tab* tab) const {
  // We don't have to clip tabs out of bounds. Scroll view will handle it.
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return true;
  }

  return TabContainerImpl::ShouldTabBeVisible(tab);
}

void BraveTabContainer::StartInsertTabAnimation(int model_index) {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::StartInsertTabAnimation(model_index);
    return;
  }

  ExitTabClosingMode();

  auto* new_tab = GetTabAtModelIndex(model_index);
  gfx::Rect bounds = new_tab->bounds();
  bounds.set_height(tabs::kVerticalTabHeight);
  const auto tab_width = new_tab->data().pinned
                             ? tabs::kVerticalTabMinWidth
                             : tab_style_->GetStandardWidth();
  bounds.set_width(tab_width);
  bounds.set_x(-tab_width);
  bounds.set_y((model_index > 0)
                   ? tabs_view_model_.ideal_bounds(model_index - 1).bottom()
                   : 0);
  GetTabAtModelIndex(model_index)->SetBoundsRect(bounds);

  // Animate in to the full width.
  AnimateToIdealBounds();
}

void BraveTabContainer::RemoveTab(int index, bool was_active) {
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    closing_tabs_.insert(tabs_view_model_.view_at(index));
  }

  TabContainerImpl::RemoveTab(index, was_active);
}

void BraveTabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    closing_tabs_.erase(tab);
  }

  TabContainerImpl::OnTabCloseAnimationCompleted(tab);

  // we might have to hide this container entirely
  if (!tabs_view_model_.view_size()) {
    PreferredSizeChanged();
  }
}

void BraveTabContainer::UpdateLayoutOrientation() {
  layout_helper_->set_use_vertical_tabs(
      tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser()));
  layout_helper_->set_tab_strip(
      static_cast<TabStrip*>(base::to_address(tab_slot_controller_)));
  InvalidateLayout();
}

void BraveTabContainer::PaintBoundingBoxForTiles(
    gfx::Canvas& canvas,
    const SplitViewBrowserData* split_view_data) {
  base::ranges::for_each(split_view_data->tiles(), [&](const auto& tile) {
    PaintBoundingBoxForTile(canvas, tile);
  });
}

void BraveTabContainer::PaintBoundingBoxForTile(gfx::Canvas& canvas,
                                                const TabTile& tile) {
  if (!GetTabCount()) {
    return;
  }

  // Note that GetTabAtModelIndex() and IsValidModelIndex() is actually using
  // "ViewModel" index. See TabContainerImpl::GetTabAtModelIndex(), and
  // implementations in compound_tab_container.cc implementation. Thus, we need
  // to add pinned tab count.
  auto* tab_strip_model = tab_slot_controller_->GetBrowser()->tab_strip_model();
  const int offset =
      IsPinnedTabContainer() ? 0 : tab_strip_model->IndexOfFirstNonPinnedTab();

  auto tab1_index = tab_strip_model->GetIndexOfTab(tile.first.Get()) - offset;
  auto tab2_index = tab_strip_model->GetIndexOfTab(tile.second.Get()) - offset;
  if (!controller_->IsValidModelIndex(tab1_index) ||
      !controller_->IsValidModelIndex(tab2_index)) {
    // In case the tiled tab is not in this container, this can happen.
    // For instance, this container is for pinned tabs but tabs in the tile are
    // unpinned.
    return;
  }

  gfx::Rect bounding_rects;
  for (auto i : {tab1_index, tab2_index}) {
    bounding_rects.Union(GetTabAtModelIndex(i)->bounds());
  }
  const bool is_vertical_tab =
      tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser());
  if (!is_vertical_tab) {
    // In order to make margin between the bounding box and tab strip.
    // Need to compensate the amount of overlap because it's hidden by overlap
    // at bottom.
    int vertical_margin = GetTabAtModelIndex(tab1_index)->data().pinned ? 4 : 2;
    bounding_rects.Inset(gfx::Insets::TLBR(
        vertical_margin, brave_tabs::kHorizontalTabInset,
        vertical_margin + GetLayoutConstant(TABSTRIP_TOOLBAR_OVERLAP),
        brave_tabs::kHorizontalTabInset));
  }

  constexpr auto kRadius = 12.f;  // same value with --leo-radius-l

  auto* cp = GetColorProvider();
  DCHECK(cp);

  cc::PaintFlags flags;
  flags.setColor(cp->GetColor(
      is_vertical_tab ? kColorBraveSplitViewTileBackgroundVertical
                      : kColorBraveSplitViewTileBackgroundHorizontal));

  canvas.DrawRoundRect(bounding_rects, kRadius, flags);

  auto active_tab_handle =
      tab_strip_model->GetTabAtIndex(tab_strip_model->active_index())
          ->GetHandle();
  if (!is_vertical_tab && active_tab_handle != tile.first &&
      active_tab_handle != tile.second &&
      !GetTabAtModelIndex(tab1_index)->IsMouseHovered() &&
      !GetTabAtModelIndex(tab2_index)->IsMouseHovered()) {
    constexpr int kSplitViewSeparatorHeight = 24;
    auto separator_top = bounding_rects.top_center();
    CHECK_GT(bounding_rects.height(), kSplitViewSeparatorHeight);
    // Calculate gap between tab bounds top and separator top.
    const int gap = (bounding_rects.height() - kSplitViewSeparatorHeight) / 2;
    separator_top.Offset(0, gap);
    auto separator_bottom = separator_top;
    separator_bottom.Offset(0, kSplitViewSeparatorHeight);
    canvas.DrawLine(separator_top, separator_bottom,
                    cp->GetColor(kColorBraveSplitViewTileDivider));
  }

  bounding_rects.Outset(1);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setColor(cp->GetColor(kColorBraveSplitViewTileBackgroundBorder));
  canvas.DrawRoundRect(bounding_rects, kRadius, flags);
}

void BraveTabContainer::OnUnlockLayout() {
  layout_locked_ = false;

  InvalidateIdealBounds();
  PreferredSizeChanged();
  CompleteAnimationAndLayout();
}

void BraveTabContainer::CompleteAnimationAndLayout() {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return;
  }

  TabContainerImpl::CompleteAnimationAndLayout();

  // Should force tabs to layout as they might not change bounds, which makes
  // insets not updated.
  base::ranges::for_each(children(), &views::View::DeprecatedLayoutImmediately);
}

void BraveTabContainer::PaintChildren(const views::PaintInfo& paint_info) {
  // Exclude tabs that own layer.
  std::vector<ZOrderableTabContainerElement> orderable_children;
  for (views::View* child : children()) {
    if (!ZOrderableTabContainerElement::CanOrderView(child)) {
      continue;
    }
    orderable_children.emplace_back(child);
  }

  std::stable_sort(orderable_children.begin(), orderable_children.end());

  if (auto* split_view_data = SplitViewBrowserData::FromBrowser(
          tab_slot_controller_->GetBrowser())) {
    ui::PaintRecorder recorder(paint_info.context(),
                               paint_info.paint_recording_size(),
                               paint_info.paint_recording_scale_x(),
                               paint_info.paint_recording_scale_y(), nullptr);
    PaintBoundingBoxForTiles(*recorder.canvas(), split_view_data);
  }

  for (const ZOrderableTabContainerElement& child : orderable_children) {
    child.view()->Paint(paint_info);
  }
}

void BraveTabContainer::SetTabSlotVisibility() {
  // During multiple tab closing including group, this method could be called
  // but group_views_ could be empty already. We should clear group info in tabs
  // in that case.
  // https://github.com/brave/brave-browser/issues/39298
  for (Tab* tab : layout_helper_->GetTabs()) {
    if (std::optional<tab_groups::TabGroupId> group = tab->group();
        group && !group_views_.contains(*group)) {
      tab->set_group(std::nullopt);
    }
  }

  TabContainerImpl::SetTabSlotVisibility();
}

std::optional<BrowserRootView::DropIndex> BraveTabContainer::GetDropIndex(
    const ui::DropTargetEvent& event) {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::GetDropIndex(event);
  }

  // Force animations to stop, otherwise it makes the index calculation tricky.
  CompleteAnimationAndLayout();

  const int x = GetMirroredXInView(event.x());
  const int y = event.y();

  std::vector<TabSlotView*> views = layout_helper_->GetTabSlotViews();

  // Loop until we find a tab or group header that intersects |event|'s
  // location.
  for (TabSlotView* view : views) {
    const int max_y = view->y() + view->height();
    const int max_x = view->x() + view->width();
    if (y >= max_y) {
      continue;
    }

    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      Tab* const tab = static_cast<Tab*>(view);

      // Closing tabs should be skipped.
      if (tab->closing()) {
        continue;
      }

      const bool is_tab_pinned = tab->data().pinned;

      // When dropping text or links onto pinned tabs, we need to take the
      // x-axis position into consideration.
      if (is_tab_pinned && x >= max_x) {
        continue;
      }

      const int model_index = GetModelIndexOf(tab).value();
      const bool first_in_group =
          tab->group().has_value() &&
          model_index == controller_->GetFirstTabInGroup(tab->group().value());

      const int hot_height = tab->height() / 4;
      const int hot_width = tab->width() / 4;

      if (is_tab_pinned ? x >= (max_x - hot_width)
                        : y >= (max_y - hot_height)) {
        return BrowserRootView::DropIndex{
            .index = model_index + 1,
            .relative_to_index = kInsertBeforeIndex,
            .group_inclusion = kDontIncludeInGroup};
      }

      if (is_tab_pinned ? x < tab->x() + hot_width
                        : y < tab->y() + hot_height) {
        return BrowserRootView::DropIndex{
            .index = model_index,
            .relative_to_index = kInsertBeforeIndex,
            .group_inclusion =
                first_in_group ? kIncludeInGroup : kDontIncludeInGroup};
      }

      return BrowserRootView::DropIndex{.index = model_index,
                                        .relative_to_index = kReplaceIndex,
                                        .group_inclusion = kIncludeInGroup};
    } else {
      TabGroupHeader* const group_header = static_cast<TabGroupHeader*>(view);
      const int first_tab_index =
          controller_->GetFirstTabInGroup(group_header->group().value())
              .value();
      return BrowserRootView::DropIndex{
          .index = first_tab_index,
          .relative_to_index = kInsertBeforeIndex,
          .group_inclusion = y >= max_y - group_header->height() / 2
                                 ? kIncludeInGroup
                                 : kDontIncludeInGroup};
    }
  }

  // The drop isn't over a tab, add it to the end.
  return BrowserRootView::DropIndex{.index = GetTabCount(),
                                    .relative_to_index = kInsertBeforeIndex,
                                    .group_inclusion = kDontIncludeInGroup};
}

// BraveTabContainer::DropArrow:
// ----------------------------------------------------------
BraveTabContainer::DropArrow::DropArrow(const BrowserRootView::DropIndex& index,
                                        Position position,
                                        bool beneath,
                                        views::Widget* context)
    : index_(index), position_(position), beneath_(beneath) {
  arrow_window_ = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.z_order = ui::ZOrderLevel::kFloatingUIElement;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.accept_events = false;

  // All drop images has the same size.
  const gfx::ImageSkia* drop_image =
      GetDropArrowImage(Position::Horizontal, false);
  params.bounds = gfx::Rect(drop_image->width(), drop_image->height());

  params.context = context->GetNativeWindow();
  arrow_window_->Init(std::move(params));
  arrow_view_ =
      arrow_window_->SetContentsView(std::make_unique<views::ImageView>());
  arrow_view_->SetImage(GetDropArrowImage(position_, beneath_));
  scoped_observation_.Observe(arrow_window_.get());

  arrow_window_->Show();
}

BraveTabContainer::DropArrow::~DropArrow() {
  // Close eventually deletes the window, which deletes arrow_view too.
  if (arrow_window_) {
    arrow_window_->Close();
  }
}

void BraveTabContainer::DropArrow::SetBeneath(bool beneath) {
  if (beneath_ == beneath) {
    return;
  }

  beneath_ = beneath;
  arrow_view_->SetImage(GetDropArrowImage(position_, beneath));
}

void BraveTabContainer::DropArrow::SetWindowBounds(const gfx::Rect& bounds) {
  arrow_window_->SetBounds(bounds);
}

void BraveTabContainer::DropArrow::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(scoped_observation_.IsObservingSource(arrow_window_.get()));
  scoped_observation_.Reset();
  arrow_window_ = nullptr;
}

void BraveTabContainer::HandleDragUpdate(
    const std::optional<BrowserRootView::DropIndex>& index) {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::HandleDragUpdate(index);
    return;
  }
  SetDropArrow(index);
}

void BraveTabContainer::HandleDragExited() {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::HandleDragExited();
    return;
  }
  SetDropArrow({});
}

void BraveTabContainer::OnTileTabs(const TabTile& tile) {
  UpdateTabsBorderInTile(tile);
  SchedulePaint();
}

void BraveTabContainer::OnDidBreakTile(const TabTile& tile) {
  UpdateTabsBorderInTile(tile);
  SchedulePaint();
}

void BraveTabContainer::OnSwapTabsInTile(const TabTile& tile) {
  UpdateTabsBorderInTile(tile);
}

gfx::Rect BraveTabContainer::GetDropBounds(int drop_index,
                                           bool drop_before,
                                           bool drop_in_group,
                                           bool* is_beneath) {
  DCHECK_NE(drop_index, -1);

  // The center is determined along the x-axis if it's pinned, or along the
  // y-axis if not.
  int center = -1;

  if (GetTabCount() == 0) {
    // If the tabstrip is empty, it doesn't matter where the drop arrow goes.
    // The tabstrip can only be transiently empty, e.g. during shutdown.
    return gfx::Rect();
  }

  Tab* tab = GetTabAtModelIndex(std::min(drop_index, GetTabCount() - 1));

  const bool is_tab_pinned = tab->data().pinned;

  const bool first_in_group =
      drop_index < GetTabCount() && tab->group().has_value() &&
      GetModelIndexOf(tab) ==
          controller_->GetFirstTabInGroup(tab->group().value());

  if (!drop_before || !first_in_group || drop_in_group) {
    // Dropping between tabs, or between a group header and the group's first
    // tab.
    center = is_tab_pinned ? tab->x() : tab->y();
    const int length = is_tab_pinned ? tab->width() : tab->height();
    if (drop_index < GetTabCount()) {
      center += drop_before ? -(tabs::kVerticalTabsSpacing / 2) : (length / 2);
    } else {
      center += length + (tabs::kVerticalTabsSpacing / 2);
    }
  } else {
    // Dropping before a group header.
    TabGroupHeader* const header = group_views_[tab->group().value()]->header();
    // Since there is no tab group in pinned tabs, there is no need to consider
    // the x-axis.
    center = header->y() + tabs::kVerticalTabsSpacing / 2;
  }

  // Since all drop indicator images are the same size, we will use the right
  // arrow image to determine the height and width.
  const gfx::ImageSkia* drop_image = GetDropArrowImage(
      BraveTabContainer::DropArrow::Position::Horizontal, false);

  // Determine the screen bounds.
  gfx::Point drop_loc(is_tab_pinned ? center - drop_image->width() / 2 : 0,
                      is_tab_pinned ? tab->y() - drop_image->height()
                                    : center - drop_image->height() / 2);
  ConvertPointToScreen(this, &drop_loc);
  gfx::Rect drop_bounds(drop_loc.x(), drop_loc.y(), drop_image->width(),
                        drop_image->height());

  // If the rect doesn't fit on the monitor, push the arrow to the bottom.
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display = screen->GetDisplayMatching(drop_bounds);
  *is_beneath = !display.bounds().Contains(drop_bounds);

  if (*is_beneath) {
    drop_bounds.Offset(
        is_tab_pinned ? 0 : drop_bounds.width() + tab->width(),
        is_tab_pinned ? drop_bounds.height() + tab->height() : 0);
  }

  return drop_bounds;
}

gfx::ImageSkia* BraveTabContainer::GetDropArrowImage(
    BraveTabContainer::DropArrow::Position pos,
    bool beneath) {
  using Position = BraveTabContainer::DropArrow::Position;
  using RotationAmount = SkBitmapOperations::RotationAmount;
  static base::NoDestructor<
      base::flat_map<std::pair<Position, bool>, gfx::ImageSkia>>
      drop_images([] {
        gfx::ImageSkia* top_arrow_image =
            ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
                IDR_TAB_DROP_UP);

        base::flat_map<std::pair<Position, bool>, gfx::ImageSkia>
            position_to_images;

        position_to_images.emplace(std::make_pair(Position::Vertical, true),
                                   *top_arrow_image);
        position_to_images.emplace(
            std::make_pair(Position::Horizontal, false),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_90_CW));
        position_to_images.emplace(
            std::make_pair(Position::Vertical, false),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_180_CW));
        position_to_images.emplace(
            std::make_pair(Position::Horizontal, true),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_270_CW));
        return position_to_images;
      }());
  return &drop_images->find(std::make_pair(pos, beneath))->second;
}

void BraveTabContainer::SetDropArrow(
    const std::optional<BrowserRootView::DropIndex>& index) {
  if (!index) {
    controller_->OnDropIndexUpdate(std::nullopt, false);
    drop_arrow_.reset();
    return;
  }

  // Let the controller know of the index update.
  controller_->OnDropIndexUpdate(
      index->index,
      index->relative_to_index ==
          BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex);

  if (drop_arrow_ && (index == drop_arrow_->index())) {
    return;
  }

  bool is_beneath = false;
  gfx::Rect drop_bounds = GetDropBounds(
      index->index,
      index->relative_to_index ==
          BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex,
      index->group_inclusion ==
          BrowserRootView::DropIndex::GroupInclusion::kIncludeInGroup,
      &is_beneath);

  if (!drop_arrow_) {
    DropArrow::Position position = DropArrow::Position::Vertical;
    if (GetTabCount() > 0) {
      Tab* tab = GetTabAtModelIndex(0);
      position = tab->data().pinned ? DropArrow::Position::Vertical
                                    : DropArrow::Position::Horizontal;
    }
    drop_arrow_ =
        std::make_unique<DropArrow>(*index, position, is_beneath, GetWidget());
  } else {
    drop_arrow_->set_index(*index);
    drop_arrow_->SetBeneath(is_beneath);
  }

  // Reposition the window.
  drop_arrow_->SetWindowBounds(drop_bounds);
}

bool BraveTabContainer::IsPinnedTabContainer() const {
  return tabs_view_model_.view_size() > 0 &&
         tabs_view_model_.view_at(0)->data().pinned;
}

void BraveTabContainer::UpdateTabsBorderInTile(const TabTile& tile) {
  auto* tab_strip_model = tab_slot_controller_->GetBrowser()->tab_strip_model();
  const int offset =
      IsPinnedTabContainer() ? 0 : tab_strip_model->IndexOfFirstNonPinnedTab();

  auto tab1_index = tab_strip_model->GetIndexOfTab(tile.first.Get()) - offset;
  auto tab2_index = tab_strip_model->GetIndexOfTab(tile.second.Get()) - offset;

  if (!controller_->IsValidModelIndex(tab1_index) ||
      !controller_->IsValidModelIndex(tab2_index)) {
    // In case the tiled tab is not in this container, this can happen.
    // For instance, this container is for pinned tabs but tabs in the tile
    // are unpinned.
    return;
  }

  auto* tab1 = GetTabAtModelIndex(tab1_index);
  auto* tab2 = GetTabAtModelIndex(tab2_index);

  // Tab's border varies per split view state.
  // See BraveVerticalTabStyle::GetContentsInsets().
  tab1->SetBorder(
      views::CreateEmptyBorder(tab1->tab_style_views()->GetContentsInsets()));
  tab2->SetBorder(
      views::CreateEmptyBorder(tab2->tab_style_views()->GetContentsInsets()));
}

BEGIN_METADATA(BraveTabContainer)
END_METADATA
