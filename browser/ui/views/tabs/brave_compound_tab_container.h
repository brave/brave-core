/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_

#include <memory>

#include "chrome/browser/ui/views/tabs/compound_tab_container.h"

class BraveCompoundTabContainer : public CompoundTabContainer {
 public:
  METADATA_HEADER(BraveCompoundTabContainer);

  BraveCompoundTabContainer(TabContainerController& controller,
                            TabHoverCardController* hover_card_controller,
                            TabDragContextBase* drag_context,
                            TabSlotController& tab_slot_controller,
                            views::View* scroll_contents_view);
  ~BraveCompoundTabContainer() override;

  // Combine results of TabContainerImpl::LockLayout() for pinned tabs and
  // un pinned tabs.
  base::OnceClosure LockLayout();

  // CompoundTabContainer:
  void SetAvailableWidthCallback(
      base::RepeatingCallback<int()> available_width_callback) override;
  void TransferTabBetweenContainers(int from_model_index,
                                    int to_model_index) override;
  void Layout() override;
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  views::SizeBounds GetAvailableSize(const views::View* child) const override;
  Tab* AddTab(std::unique_ptr<Tab> tab,
              int model_index,
              TabPinned pinned) override;
  int GetUnpinnedContainerIdealLeadingX() const override;
  TabContainer* GetTabContainerAt(
      gfx::Point point_in_local_coords) const override;
  gfx::Rect ConvertUnpinnedContainerIdealBoundsToLocal(
      gfx::Rect ideal_bounds) const override;
  BrowserRootView::DropTarget* GetDropTarget(
      gfx::Point loc_in_local_coords) override;
  void OnThemeChanged() override;

  bool ShouldShowVerticalTabs() const;

  base::raw_ref<TabSlotController> tab_slot_controller_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_COMPOUND_TAB_CONTAINER_H_
