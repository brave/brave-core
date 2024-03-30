/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_browser_data.h"

#include "base/check_is_test.h"
#include "base/notreached.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"

SplitViewBrowserData::SplitViewBrowserData(Browser* browser)
    : BrowserUserData(*browser) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  tab_strip_model_adapter_ = std::make_unique<SplitViewTabStripModelAdapter>(
      *this, browser->tab_strip_model());
}

SplitViewBrowserData::~SplitViewBrowserData() = default;

void SplitViewBrowserData::TileTabs(const Tile& tile) {
  CHECK(!IsTabTiled(tile.first));
  CHECK(!IsTabTiled(tile.second));

  tiles_.push_back(tile);

  tile_index_for_tab_[tile.first] = tiles_.size() - 1;
  tile_index_for_tab_[tile.second] = tiles_.size() - 1;

  if (tab_strip_model_adapter_) {
    tab_strip_model_adapter_->MakeTiledTabsAdjacent(tile);
  }
}

void SplitViewBrowserData::BreakTile(const tabs::TabHandle& tab) {
  if (auto iter = FindTile(tab); iter != tiles_.end()) {
    auto index = tile_index_for_tab_[iter->first];
    tile_index_for_tab_.erase(iter->first);
    tile_index_for_tab_.erase(iter->second);
    for (auto& [t, tile_index] : tile_index_for_tab_) {
      if (tile_index > index) {
        tile_index--;
      }
    }

    tiles_.erase(iter);
  } else {
    NOTREACHED() << "Tile doesn't exist";
  }
}

std::vector<SplitViewBrowserData::Tile>::const_iterator
SplitViewBrowserData::FindTile(const tabs::TabHandle& tab) {
  if (IsTabTiled(tab)) {
    return tiles_.begin() + tile_index_for_tab_[tab];
  }

  return tiles_.end();
}

std::vector<SplitViewBrowserData::Tile>::const_iterator
SplitViewBrowserData::FindTile(const tabs::TabHandle& tab) const {
  return const_cast<SplitViewBrowserData*>(this)->FindTile(tab);
}

bool SplitViewBrowserData::IsTabTiled(const tabs::TabHandle& tab) const {
  return base::Contains(tile_index_for_tab_, tab);
}

std::optional<SplitViewBrowserData::Tile> SplitViewBrowserData::GetTile(
    const tabs::TabHandle& tab) const {
  auto iter = FindTile(tab);
  if (iter == tiles_.end()) {
    return std::nullopt;
  }
  return *iter;
}

BROWSER_USER_DATA_KEY_IMPL(SplitViewBrowserData);
