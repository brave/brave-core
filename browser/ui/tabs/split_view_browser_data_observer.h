/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_
#define BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_

#include "base/observer_list_types.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"

class SplitViewBrowserDataObserver : public base::CheckedObserver {
 public:
  virtual void OnTileTabs(const SplitViewBrowserData::Tile& tile) {}
  virtual void OnWillBreakTile(const SplitViewBrowserData::Tile& tile) {}
  virtual void OnDidBreakTile(const SplitViewBrowserData::Tile& tile) {}
  virtual void OnSwapTabsInTile(const SplitViewBrowserData::Tile& tile) {}
  virtual void OnWillDeleteBrowserData() {}
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_
