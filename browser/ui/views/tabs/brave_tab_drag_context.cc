/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_drag_context.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_slot_view.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

BraveTabDragContext::BraveTabDragContext(TabStrip* strip,
                                         TabDragContext* original_context)
    : tab_strip_(strip), original_context_(original_context) {}

BraveTabDragContext::~BraveTabDragContext() = default;

views::View* BraveTabDragContext::AsView() {
  return original_context_->AsView();
}

const views::View* BraveTabDragContext::AsView() const {
  return original_context_->AsView();
}

Tab* BraveTabDragContext::GetTabAt(int index) const {
  return original_context_->GetTabAt(index);
}

int BraveTabDragContext::GetIndexOf(const TabSlotView* view) const {
  return original_context_->GetIndexOf(view);
}

int BraveTabDragContext::GetTabCount() const {
  return original_context_->GetTabCount();
}

bool BraveTabDragContext::IsTabPinned(const Tab* tab) const {
  return original_context_->IsTabPinned(tab);
}

int BraveTabDragContext::BraveTabDragContext::GetPinnedTabCount() const {
  return original_context_->GetPinnedTabCount();
}

TabGroupHeader* BraveTabDragContext::GetTabGroupHeader(
    const tab_groups::TabGroupId& group) const {
  return original_context_->GetTabGroupHeader(group);
}

TabStripModel* BraveTabDragContext::GetTabStripModel() {
  return original_context_->GetTabStripModel();
}

TabDragController* BraveTabDragContext::GetDragController() {
  return original_context_->GetDragController();
}

void BraveTabDragContext::OwnDragController(
    std::unique_ptr<TabDragController> controller) {
  original_context_->OwnDragController(std::move(controller));
}

[[nodiscard]] std::unique_ptr<TabDragController>
BraveTabDragContext::ReleaseDragController() {
  return original_context_->ReleaseDragController();
}

void BraveTabDragContext::SetDragControllerCallbackForTesting(
    base::OnceCallback<void(TabDragController*)> callback) {
  return original_context_->SetDragControllerCallbackForTesting(
      std::move(callback));
}

void BraveTabDragContext::DestroyDragController() {
  original_context_->DestroyDragController();
}

bool BraveTabDragContext::IsDragSessionActive() const {
  return original_context_->IsDragSessionActive();
}

bool BraveTabDragContext::IsActiveDropTarget() const {
  return original_context_->IsActiveDropTarget();
}

std::vector<int> BraveTabDragContext::GetTabXCoordinates() const {
  return original_context_->GetTabXCoordinates();
}

int BraveTabDragContext::BraveTabDragContext::GetActiveTabWidth() const {
  return original_context_->GetActiveTabWidth();
}

int BraveTabDragContext::BraveTabDragContext::GetTabDragAreaWidth() const {
  return original_context_->GetTabDragAreaWidth();
}

int BraveTabDragContext::BraveTabDragContext::TabDragAreaEndX() const {
  return original_context_->TabDragAreaEndX();
}

int BraveTabDragContext::BraveTabDragContext::TabDragAreaBeginX() const {
  return original_context_->TabDragAreaBeginX();
}

int BraveTabDragContext::BraveTabDragContext::GetHorizontalDragThreshold()
    const {
  return original_context_->GetHorizontalDragThreshold();
}

void BraveTabDragContext::SetBoundsForDrag(
    const std::vector<TabSlotView*>& views,
    const std::vector<gfx::Rect>& bounds) {
  return original_context_->SetBoundsForDrag(views, bounds);
}

void BraveTabDragContext::StartedDragging(
    const std::vector<TabSlotView*>& views) {
  return original_context_->StartedDragging(views);
}

void BraveTabDragContext::DraggedTabsDetached() {
  original_context_->DraggedTabsDetached();
}

void BraveTabDragContext::StoppedDragging(
    const std::vector<TabSlotView*>& views,
    const std::vector<int>& initial_positions,
    bool completed) {
  original_context_->StoppedDragging(views, initial_positions, completed);
}

void BraveTabDragContext::LayoutDraggedViewsAt(
    const std::vector<TabSlotView*>& views,
    TabSlotView* source_view,
    const gfx::Point& location,
    bool initial_drag) {
  original_context_->LayoutDraggedViewsAt(views, source_view, location,
                                          initial_drag);
}

void BraveTabDragContext::ForceLayout() {
  original_context_->ForceLayout();
}

int BraveTabDragContext::GetInsertionIndexForDraggedBounds(
    const gfx::Rect& dragged_bounds,
    std::vector<TabSlotView*> dragged_views,
    int num_dragged_tabs,
    absl::optional<tab_groups::TabGroupId> group) const {
  if (!tabs::features::ShouldShowVerticalTabs()) {
    return original_context_->GetInsertionIndexForDraggedBounds(
        dragged_bounds, dragged_views, num_dragged_tabs, group);
  }

  // The implementation of this method is based on
  // TabDragContext::GetInsertionIndexForDraggedBounds().
  if (!GetTabCount())
    return 0;

  absl::optional<int> index;
  // If we're dragging a group by its header, the first element of
  // |dragged_views| is a group header, and the second one is the first tab
  // in that group.
  int first_dragged_tab_index = group.has_value() ? 1 : 0;
  if (static_cast<size_t>(first_dragged_tab_index) < dragged_views.size()) {
    int first_dragged_tab_model_index =
        tab_strip_->GetModelIndexOf(dragged_views[first_dragged_tab_index]);
    index =
        CalculateInsertionIndex(dragged_bounds, first_dragged_tab_model_index,
                                num_dragged_tabs, std::move(group));
  }

  if (!index) {
    const int last_tab_right =
        tab_strip_->ideal_bounds(GetTabCount() - 1).right();
    index = (dragged_bounds.right() > last_tab_right) ? GetTabCount() : 0;
  }

  const Tab* last_visible_tab = tab_strip_->GetLastVisibleTab();
  int last_insertion_point =
      last_visible_tab ? (GetIndexOf(last_visible_tab) + 1) : 0;

  // Clamp the insertion point to keep it within the visible region.
  last_insertion_point = std::max(0, last_insertion_point - num_dragged_tabs);

  // Ensure the first dragged tab always stays in the visible index range.
  return std::min(*index, last_insertion_point);
}

std::vector<gfx::Rect> BraveTabDragContext::CalculateBoundsForDraggedViews(
    const std::vector<TabSlotView*>& views) {
  if (tabs::features::ShouldShowVerticalTabs()) {
    std::vector<gfx::Rect> bounds;
    int y = 0;
    for (const TabSlotView* view : views) {
      const int height = view->height();
      bounds.push_back(gfx::Rect(0, y, view->width(), view->height()));
      y += height;
    }
    return bounds;
  }

  return original_context_->CalculateBoundsForDraggedViews(views);
}

int BraveTabDragContext::CalculateInsertionIndex(
    const gfx::Rect& dragged_bounds,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    absl::optional<tab_groups::TabGroupId> dragged_group) const {
  DCHECK(tabs::features::ShouldShowVerticalTabs());

  // Most of this logic is borrowed from TabDragContextImpl
  int min_distance_index = -1;
  int min_distance = std::numeric_limits<int>::max();
  for (int candidate_index = 0; candidate_index <= GetTabCount();
       ++candidate_index) {
    if (!IsValidInsertionIndex(candidate_index, first_dragged_tab_index,
                               num_dragged_tabs, dragged_group)) {
      continue;
    }

    const int ideal_y =
        candidate_index == 0
            ? 0
            : tab_strip_->ideal_bounds(candidate_index - 1).bottom();
    const int distance = std::abs(dragged_bounds.y() - ideal_y);
    if (distance < min_distance) {
      min_distance = distance;
      min_distance_index = candidate_index;
    }
  }

  if (min_distance_index == -1) {
    NOTREACHED();
    return 0;
  }

  // When moving a tab within a tabstrip, the target index is expressed as if
  // the tabs are not in the tabstrip, i.e. it acts like the tabs are first
  // removed and then re-inserted at the target index. We need to adjust the
  // target index to account for this.
  if (min_distance_index > first_dragged_tab_index)
    min_distance_index -= num_dragged_tabs;

  return min_distance_index;
}

bool BraveTabDragContext::IsValidInsertionIndex(
    int candidate_index,
    int first_dragged_tab_index,
    int num_dragged_tabs,
    absl::optional<tab_groups::TabGroupId> dragged_group) const {
  if (candidate_index == 0)
    return true;

  // If |candidate_index| is right after one of the tabs we're dragging,
  // inserting here would be nonsensical - we can't insert the dragged tabs
  // into the middle of the dragged tabs. That's just silly.
  if (candidate_index > first_dragged_tab_index &&
      candidate_index <= first_dragged_tab_index + num_dragged_tabs) {
    return false;
  }

  // This might be in the middle of a group, which may or may not be fine.
  absl::optional<tab_groups::TabGroupId> left_group =
      GetTabAt(candidate_index - 1)->group();
  absl::optional<tab_groups::TabGroupId> right_group =
      tab_strip_->IsValidModelIndex(candidate_index)
          ? GetTabAt(candidate_index)->group()
          : absl::nullopt;
  if (left_group.has_value() && left_group == right_group) {
    // Can't drag a group into another group.
    if (dragged_group.has_value())
      return false;
    // Can't drag a tab into a collapsed group.
    if (tab_strip_->IsGroupCollapsed(left_group.value()))
      return false;
  }

  return true;
}
