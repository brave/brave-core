/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_CONTROLLER_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_CONTROLLER_H_

#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "components/tab_collections/public/tab_interface.h"

class SplitViewBrowserData;
class SplitView;
class TabStripModel;
struct TabTile;

namespace content {
class WebContents;
}  // namespace content

// Determine which tab contents should be shown in split view based on
// models(ex, TabTile data) and notifies to split view when it should
// be updated.
class SplitViewController : public SplitViewBrowserDataObserver {
 public:
  explicit SplitViewController(TabStripModel* tab_strip_model);
  ~SplitViewController() override;

  SplitViewBrowserData* split_view_browser_data() {
    return &split_view_tab_tile_data_;
  }

  void set_split_view(SplitView* view) { view_ = view; }

  // true when active tab is opened in split view.
  bool IsSplitViewActive() const;

  // true when |contents| is opened in split view.
  bool IsOpenedFor(content::WebContents* contents) const;

  // true when |contents[0]| and |contents[1]| are showing together in split
  // view regardless of its active state.
  bool AreShowingTogether(const std::vector<content::WebContents*>& contents);
  bool ShouldShowActiveWebContentsAtRight() const;
  content::WebContents* GetNonActiveWebContents() const;
  void SetSizeDeltaFor(content::WebContents* contents, int delta);
  int GetSizeDeltaFor(content::WebContents* contents);

 private:
  std::optional<TabTile> GetActiveTabTile() const;
  tabs::TabHandle GetActiveTabHandle() const;
  tabs::TabHandle GetTabHandleFor(content::WebContents* contents) const;
  std::optional<TabTile> GetTileFor(content::WebContents* contents) const;

  // true when |tile| has active web contents.
  bool IsActiveWebContentsIncludedIn(const TabTile& tile) const;

  // SplitViewBrowserDataObserver:
  void OnTileTabs(const TabTile& tile) override;
  void OnDidBreakTile(const TabTile& tile) override;
  void OnSwapTabsInTile(const TabTile& tile) override;

  // View for split view.
  raw_ptr<SplitView> view_ = nullptr;

  SplitViewBrowserData split_view_tab_tile_data_;

  raw_ref<TabStripModel> tab_strip_model_;
  base::ScopedObservation<SplitViewBrowserData, SplitViewBrowserDataObserver>
      split_view_tab_tile_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_CONTROLLER_H_
