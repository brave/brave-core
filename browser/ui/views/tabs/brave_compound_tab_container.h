/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_

#include <memory>
#include <optional>

#include "chrome/browser/ui/views/tabs/compound_tab_container.h"

namespace views {
class ScrollView;
}  // namespace views

class BraveCompoundTabContainer : public CompoundTabContainer {
  METADATA_HEADER(BraveCompoundTabContainer, CompoundTabContainer)
 public:

  BraveCompoundTabContainer(TabContainerController& controller,
                            TabHoverCardController* hover_card_controller,
                            TabDragContextBase* drag_context,
                            TabSlotController& tab_slot_controller,
                            views::View* scroll_contents_view);
  ~BraveCompoundTabContainer() override;

  // Combine results of TabContainerImpl::LockLayout() for pinned tabs and
  // un pinned tabs.
  base::OnceClosure LockLayout();

  void SetScrollEnabled(bool enabled);

  // CompoundTabContainer:
  void SetAvailableWidthCallback(
      base::RepeatingCallback<int()> available_width_callback) override;
  void TransferTabBetweenContainers(int from_model_index,
                                    int to_model_index) override;
  void Layout(PassKey) override;
  gfx::Size CalculatePreferredSize(
      const views::SizeBounds& available_size) const override;
  gfx::Size GetMinimumSize() const override;
  views::SizeBounds GetAvailableSize(const views::View* child) const override;
  Tab* AddTab(std::unique_ptr<Tab> tab,
              int model_index,
              TabPinned pinned) override;
  void MoveTab(int from_model_index, int to_model_index) override;
  void RemoveTab(int index, bool was_active) override;
  void SetTabPinned(int model_index, TabPinned pinned) override;
  int GetUnpinnedContainerIdealLeadingX() const override;
  TabContainer* GetTabContainerAt(
      gfx::Point point_in_local_coords) const override;
  gfx::Rect ConvertUnpinnedContainerIdealBoundsToLocal(
      gfx::Rect ideal_bounds) const override;
  void PaintChildren(const views::PaintInfo& info) override;
  void ChildPreferredSizeChanged(views::View* child) override;
  void SetActiveTab(std::optional<size_t> prev_active_index,
                    std::optional<size_t> new_active_index) override;
  views::View* TargetForRect(views::View* root, const gfx::Rect& rect) override;

  // BrowserRootView::DropTarget
  BrowserRootView::DropTarget* GetDropTarget(
      gfx::Point loc_in_local_coords) override;
  std::optional<BrowserRootView::DropIndex> GetDropIndex(
      const ui::DropTargetEvent& event) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollBarVisibility);

  bool ShouldShowVerticalTabs() const;

  void UpdatePinnedTabContainerBorder();
  void UpdateUnpinnedContainerSize();
  void ScrollTabToBeVisible(int model_index);

  int GetAvailableWidthConsideringScrollBar();

  raw_ref<TabSlotController> tab_slot_controller_;

  raw_ptr<views::ScrollView, DanglingUntriaged> scroll_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
