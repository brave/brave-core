/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_DRAG_CONTEXT_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_DRAG_CONTEXT_H_

#include <memory>
#include <vector>

#include "chrome/browser/ui/tabs/tab_group.h"  // tab_drg_context.h missing this.
#include "chrome/browser/ui/views/tabs/tab_drag_context.h"

class BraveTabDragContext : public TabDragContext {
 public:
  BraveTabDragContext(TabStrip* strip, TabDragContext* original_context);
  ~BraveTabDragContext() override;

  // Use |original_context_|'s implementation
  views::View* AsView() override;
  const views::View* AsView() const override;
  Tab* GetTabAt(int index) const override;
  int GetIndexOf(const TabSlotView* view) const override;
  int GetTabCount() const override;
  bool IsTabPinned(const Tab* tab) const override;
  int GetPinnedTabCount() const override;
  TabGroupHeader* GetTabGroupHeader(
      const tab_groups::TabGroupId& group) const override;
  TabStripModel* GetTabStripModel() override;
  TabDragController* GetDragController() override;
  void OwnDragController(
      std::unique_ptr<TabDragController> controller) override;
  [[nodiscard]] std::unique_ptr<TabDragController> ReleaseDragController()
      override;
  void SetDragControllerCallbackForTesting(
      base::OnceCallback<void(TabDragController*)> callback) override;
  void DestroyDragController() override;
  bool IsDragSessionActive() const override;
  bool IsActiveDropTarget() const override;
  std::vector<int> GetTabXCoordinates() const override;
  int GetActiveTabWidth() const override;
  int GetTabDragAreaWidth() const override;
  int TabDragAreaEndX() const override;
  int TabDragAreaBeginX() const override;
  int GetHorizontalDragThreshold() const override;
  void SetBoundsForDrag(const std::vector<TabSlotView*>& views,
                        const std::vector<gfx::Rect>& bounds) override;
  void StartedDragging(const std::vector<TabSlotView*>& views) override;
  void DraggedTabsDetached() override;
  void StoppedDragging(const std::vector<TabSlotView*>& views,
                       const std::vector<int>& initial_positions,
                       bool completed) override;
  void LayoutDraggedViewsAt(const std::vector<TabSlotView*>& views,
                            TabSlotView* source_view,
                            const gfx::Point& location,
                            bool initial_drag) override;
  void ForceLayout() override;

  // Specialized implementations for vertical tabs
  int GetInsertionIndexForDraggedBounds(
      const gfx::Rect& dragged_bounds,
      std::vector<TabSlotView*> dragged_views,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> group) const override;
  std::vector<gfx::Rect> CalculateBoundsForDraggedViews(
      const std::vector<TabSlotView*>& views) override;

 private:
  int CalculateInsertionIndex(
      const gfx::Rect& dragged_bounds,
      int first_dragged_tab_index,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> dragged_group) const;
  bool IsValidInsertionIndex(
      int candidate_index,
      int first_dragged_tab_index,
      int num_dragged_tabs,
      absl::optional<tab_groups::TabGroupId> dragged_group) const;

  raw_ptr<TabStrip> tab_strip_ = nullptr;
  raw_ptr<TabDragContext> original_context_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_DRAG_CONTEXT_H_
