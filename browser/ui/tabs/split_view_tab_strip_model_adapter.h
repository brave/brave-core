/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_TAB_STRIP_MODEL_ADAPTER_H_
#define BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_TAB_STRIP_MODEL_ADAPTER_H_

#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class SplitViewBrowserData;
struct TabTile;

// This class observes changes in tabs' index and make other tab that are paired
// with the changed tab be synchronized.
class SplitViewTabStripModelAdapter : public TabStripModelObserver {
 public:
  SplitViewTabStripModelAdapter(SplitViewBrowserData& split_view_browser_data,
                                TabStripModel* model);
  ~SplitViewTabStripModelAdapter() override;

  void MakeTiledTabsAdjacent(const TabTile& tile, bool move_right_tab = true);
  bool SynchronizeGroupedState(const TabTile& tile,
                               const tabs::TabHandle& source,
                               std::optional<tab_groups::TabGroupId> group);
  bool SynchronizePinnedState(const TabTile& tile,
                              const tabs::TabHandle& source);

  void TabDragStarted();
  void TabDragEnded();
  bool is_in_tab_dragging() const { return is_in_tab_dragging_; }
  TabStripModel& tab_strip_model() { return *model_; }

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabWillBeRemoved(content::WebContents* contents, int index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabGroupedStateChanged(std::optional<tab_groups::TabGroupId> group,
                              tabs::TabInterface* tab,
                              int index) override;

 private:
  void OnTabInserted(const TabStripModelChange::Insert* insert);
  void OnTabMoved(const TabStripModelChange::Move* move);
  void OnTabRemoved(const TabStripModelChange::Remove* remove);

  // Filled up in OnTabWillBeRemoved() and revisited from OnTabRemoved().
  // These are in pending state until we can decide what to do with them in
  // OnTabRemoved() by looking into the reason.
  std::vector<std::pair<content::WebContents*, content::WebContents*>>
      tiled_tabs_scheduled_to_be_removed_;

  raw_ref<SplitViewBrowserData> split_view_browser_data_;  // owner
  raw_ref<TabStripModel, DanglingUntriaged> model_;

  bool is_in_tab_dragging_ = false;

  bool is_in_synch_grouped_state_ = false;

  base::WeakPtrFactory<SplitViewTabStripModelAdapter> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_TAB_STRIP_MODEL_ADAPTER_H_
