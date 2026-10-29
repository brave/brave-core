/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#include <algorithm>
#include <optional>

#include "base/check.h"
#include "brave/browser/ui/tabs/brave_tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/tabs/dragging/drag_session_data.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "ui/gfx/geometry/point.h"

DraggingTabsSession::DraggingTabsSession(
    DragSessionData drag_data,
    TabDragContext& attached_context,
    TabDragPositioningDelegate& drag_position_delegate,
    bool initial_move,
    gfx::Point point_in_screen)
    : DraggingTabsSessionChromium(drag_data,
                                  attached_context,
                                  drag_position_delegate,
                                  initial_move,
                                  point_in_screen) {}

DraggingTabsSession::~DraggingTabsSession() {}

gfx::Point DraggingTabsSession::GetAttachedDragPoint(
    gfx::Point point_in_screen) {
  if (!is_showing_vertical_tabs_) {
    return DraggingTabsSessionChromium::GetAttachedDragPoint(point_in_screen);
  }

  gfx::Point tab_loc(point_in_screen);
  views::View::ConvertPointFromScreen(base::to_address(attached_context_),
                                      &tab_loc);
  const int x = drag_data_.tab_drag_data_.front().pinned
                    ? tab_loc.x() - mouse_offset_
                    : 0;
  const int y = tab_loc.y() - mouse_y_offset_;
  return {x, y};
}

void DraggingTabsSession::MoveAttached(gfx::Point point_in_screen) {
  if (!CanAdjustTreeTabNestingLevel()) {
    DraggingTabsSessionChromium::MoveAttached(point_in_screen);
  } else {
    // Calculate the destination the same way upstream does, so that we can
    // pass the nesting hint for the move.
    const gfx::Rect dragged_bounds =
        GetDraggedViewTabStripBounds(GetAttachedDragPoint(point_in_screen));
    const int to_index =
        drag_position_delegate_->GetInsertionIndexForDraggedBounds(
            dragged_bounds, drag_data_.attached_views(),
            drag_data_.num_dragging_tabs());
    const auto drag_target =
        CalculateTreeTabDragTarget(dragged_bounds, to_index);

    auto* model =
        static_cast<BraveTabStripModel*>(attached_context_->GetTabStripModel());
    model->SetTreeTabDragTarget(drag_target);
    DraggingTabsSessionChromium::MoveAttached(point_in_screen);
    if (drag_target != last_tree_tab_drag_target_) {
      // Upstream moves tabs only when the index changes enough, but the
      // nesting level can change without index change. Move again so that
      // the new hint is applied.
      model->MoveSelectedTabsTo(to_index,
                                CalculateGroupForDraggedTabs(to_index));
      last_tree_tab_drag_target_ = drag_target;
    }
    model->SetTreeTabDragTarget(std::nullopt);
  }

  if (!is_showing_vertical_tabs_) {
    return;
  }

  // Unlike upstream, We always update coordinate, as we use y coordinate. Since
  // we don't have threshold there's no any harm for this.
  views::View::ConvertPointFromScreen(base::to_address(attached_context_),
                                      &point_in_screen);
  last_move_attached_context_loc_ = point_in_screen.y();
}

bool DraggingTabsSession::CanAdjustTreeTabNestingLevel() const {
  if (!is_showing_vertical_tabs_ ||
      drag_data_.group_header_drag_data_.has_value()) {
    return false;
  }

  auto* model =
      static_cast<BraveTabStripModel*>(attached_context_->GetTabStripModel());
  if (!model->tree_model()) {
    return false;
  }

  return std::ranges::none_of(
      drag_data_.tab_drag_data_,
      [](const TabDragData& data) { return data.pinned; });
}

std::optional<TreeTabDragTarget>
DraggingTabsSession::CalculateTreeTabDragTarget(const gfx::Rect& dragged_bounds,
                                                int to_index) {
  // |to_index| is the index of the first dragged tab after the move. So the
  // tabs right above and below the drop position are the (to_index - 1)th and
  // the (to_index)th ones among tabs that are not moved.
  TabStripModel* model = attached_context_->GetTabStripModel();
  const auto& selection = model->selection_model().GetListSelectionModel();
  int above_model_index = TabStripModel::kNoTab;
  int below_model_index = TabStripModel::kNoTab;
  for (int i = 0, non_moving_count = 0; i < model->count(); ++i) {
    if (selection.IsSelected(i)) {
      continue;
    }
    if (non_moving_count == to_index - 1) {
      above_model_index = i;
    } else if (non_moving_count == to_index) {
      below_model_index = i;
      break;
    }
    ++non_moving_count;
  }

  // Sibling of the tab below. When there's no tab below, the target is empty,
  // which means becoming a top-level node at the end.
  TreeTabDragTarget sibling_of_below{TreeTabDragHint::kSibling, {}};
  if (below_model_index != TabStripModel::kNoTab) {
    sibling_of_below.target =
        model->GetTabAtIndex(below_model_index)->GetHandle();
  }

  if (above_model_index == TabStripModel::kNoTab ||
      model->IsTabPinned(above_model_index)) {
    return sibling_of_below;
  }

  // The dragged views are within half a tab height of the bottom of the tab
  // above, otherwise |to_index| would be different. Split that range into
  // three zones.
  const TabSlotView* above_view =
      drag_position_delegate_->GetTabAt(above_model_index);
  const int height = above_view->height();
  if (height <= 0) {
    return std::nullopt;
  }
  const int delta = dragged_bounds.y() - above_view->bounds().bottom();
  const int zone_threshold = height / 6;
  if (delta > zone_threshold) {
    return sibling_of_below;
  }

  const tabs::TabHandle above =
      model->GetTabAtIndex(above_model_index)->GetHandle();
  if (delta < -zone_threshold) {
    return TreeTabDragTarget{TreeTabDragHint::kSibling, above};
  }
  return TreeTabDragTarget{TreeTabDragHint::kChild, above};
}

std::optional<tab_groups::TabGroupId>
DraggingTabsSession::CalculateGroupForDraggedTabs(int to_index) {
  if (!is_showing_vertical_tabs_) {
    return DraggingTabsSessionChromium::CalculateGroupForDraggedTabs(to_index);
  }

  TabStripModel* attached_model = attached_context_->GetTabStripModel();

  // If a group is moved, the drag cannot be inserted into another group.
  for (const TabDragData& tab_drag_datum : drag_data_.tab_drag_data_) {
    if (tab_drag_datum.view_type == TabSlotView::ViewType::kTabGroupHeader) {
      return std::nullopt;
    }
  }

  // Pinned tabs cannot be grouped, so we only change the group membership of
  // unpinned tabs.
  std::vector<int> selected_unpinned;
  for (size_t selected_index : attached_model->selection_model()
                                   .GetListSelectionModel()
                                   .selected_indices()) {
    if (!attached_model->IsTabPinned(selected_index)) {
      selected_unpinned.push_back(selected_index);
    }
  }

  if (selected_unpinned.empty()) {
    return std::nullopt;
  }

  // Get the proposed tabstrip model assuming the selection has taken place.
  auto [previous_index, next_index] =
      attached_model->GetAdjacentTabsAfterSelectedMove(GetPassKey(), to_index);
  std::optional<tab_groups::TabGroupId> previous_group =
      previous_index.has_value()
          ? attached_model->GetTabGroupForTab(previous_index.value())
          : std::nullopt;
  std::optional<tab_groups::TabGroupId> next_group =
      next_index.has_value()
          ? attached_model->GetTabGroupForTab(next_index.value())
          : std::nullopt;
  std::optional<tab_groups::TabGroupId> current_group =
      attached_model->GetTabGroupForTab(selected_unpinned[0]);

  // We're in the middle of two tabs with the same group membership, or both
  // sides are ungrouped.
  if (previous_group == next_group) {
    return previous_group;
  }

  // If the tabs on the previous and next have different group memberships,
  // including if one is ungrouped or nonexistent, change the group of the
  // dragged tab based on whether it is "leaning" toward the previous or the
  // next of the gap. If the tab is centered in the gap, make the tab
  // ungrouped.

  const TabSlotView* top_most_selected_tab =
      drag_position_delegate_->GetTabAt(selected_unpinned[0]);

  const int buffer = top_most_selected_tab->height() / 4;

  const auto tab_bounds_in_drag_context_coords = [this](int model_index) {
    const TabSlotView* const tab =
        drag_position_delegate_->GetTabAt(model_index);
    return ToEnclosingRect(views::View::ConvertRectToTarget(
        tab->parent(), base::to_address(attached_context_),
        gfx::RectF(tab->bounds())));
  };

  // Use the top edge for a reliable fallback, e.g. if this is the topmost
  // tab or there is a group header to the immediate top.
  int top_edge =
      previous_index.has_value()
          ? tab_bounds_in_drag_context_coords(previous_index.value()).bottom()
          : 0;

  // Extra polish: Prefer staying in an existing group, if any. This prevents
  // tabs at the edge of the group from flickering between grouped and
  // ungrouped. It also gives groups a slightly "sticky" feel while dragging.
  if (previous_group.has_value() && previous_group == current_group) {
    top_edge += buffer;
  }
  if (next_group.has_value() && next_group == current_group && top_edge > 0) {
    top_edge -= buffer;
  }

  const int top_most_selected_y_position = top_most_selected_tab->y();

  if (previous_group.has_value() &&
      !attached_model->IsGroupCollapsed(previous_group.value()) &&
      top_most_selected_y_position <= top_edge - buffer) {
    return previous_group;
  }
  if ((top_most_selected_y_position >= top_edge + buffer) &&
      next_group.has_value() &&
      !attached_model->IsGroupCollapsed(next_group.value())) {
    return next_group;
  }
  return std::nullopt;
}
