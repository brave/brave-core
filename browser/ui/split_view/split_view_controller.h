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
#include "brave/browser/ui/split_view/split_view_web_panel_data.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "components/tab_collections/public/tab_interface.h"

class SplitViewBrowserData;
class SplitViewView;
class SplitViewWebPanelData;
class TabStripModel;
struct TabTile;

namespace content {
class WebContents;
}  // namespace content

// Control split view state based on two models(split view tab tile and web
// panel data).
// TODO(https://github.com/brave/brave-browser/issues/45475):
// SplitViewWebPanelData is not implemented yet.
class SplitViewController : public SplitViewBrowserDataObserver {
 public:
  explicit SplitViewController(TabStripModel* tab_strip_model);
  ~SplitViewController() override;

  SplitViewBrowserData* split_view_browser_data() {
    return &split_view_tab_tile_data_;
  }

  SplitViewWebPanelData* split_view_web_panel_data() {
    return &split_view_web_panel_data_;
  }

  void set_split_view_view(SplitViewView* view) {
    view_ = view;
    split_view_web_panel_data_.view_ = view;
  }

  // true when active tab is opened in split view.
  bool IsSplitViewActive() const;

  // true when |content| is opened in split view.
  bool IsOpenedFor(content::WebContents* contents) const;

  // true when |contents[0]| and |contents[1]| are showing together in split
  // view regardless of its active state.
  bool AreShowingTogether(const std::vector<content::WebContents*>& contents);
  bool ShouldShowActiveWebContentsAtRight() const;
  content::WebContents* GetNonActiveWebContents() const;
  void CacheSizeDeltaFor(content::WebContents* contents, int delta);
  int GetSizeDeltaFor(content::WebContents* contents);
  void WillChangeActiveWebContents(content::WebContents* old_contents,
                                   content::WebContents* new_contents);

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
  raw_ptr<SplitViewView> view_ = nullptr;

  // Two models for split view.
  SplitViewBrowserData split_view_tab_tile_data_;
  SplitViewWebPanelData split_view_web_panel_data_;

  raw_ref<TabStripModel> tab_strip_model_;
  base::ScopedObservation<SplitViewBrowserData, SplitViewBrowserDataObserver>
      split_view_tab_tile_observation_{this};
};

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_CONTROLLER_H_
