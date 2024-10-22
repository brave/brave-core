/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_
#define BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_

#include "base/observer_list_types.h"

struct TabTile;

class SplitViewBrowserDataObserver : public base::CheckedObserver {
 public:
  virtual void OnTileTabs(const TabTile& tile) {}
  virtual void OnWillBreakTile(const TabTile& tile) {}
  virtual void OnDidBreakTile(const TabTile& tile) {}
  virtual void OnSwapTabsInTile(const TabTile& tile) {}
  virtual void OnWillDeleteBrowserData() {}
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_OBSERVER_H_
