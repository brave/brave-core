/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_browser_data.h"

#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/notreached.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"
#include "chrome/browser/ui/tabs/tab_model.h"

SplitViewBrowserData::SplitViewBrowserData(Browser* browser)
    : BrowserUserData(*browser),
      tab_strip_model_adapter_(*this, browser->tab_strip_model()) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));
}

SplitViewBrowserData::~SplitViewBrowserData() {
  // base::ObserverList is safe to be mutated during iteration.
  for (auto& observer : observers_) {
    observer.OnWillDeleteBrowserData();
  }
}

void SplitViewBrowserData::TileTabs(const TabTile& tile) {
  CHECK(!IsTabTiled(tile.first));
  CHECK(!IsTabTiled(tile.second));

  auto& model = tab_strip_model_adapter_.tab_strip_model();
  CHECK_LT(model.GetIndexOfTab(tile.first), model.GetIndexOfTab(tile.second));

  tiles_.push_back(tile);

  tile_index_for_tab_[tile.first] = tiles_.size() - 1;
  tile_index_for_tab_[tile.second] = tiles_.size() - 1;

  bool tabs_are_adjacent =
      tab_strip_model_adapter_.SynchronizePinnedState(tile, tile.first);
  tabs_are_adjacent |= tab_strip_model_adapter_.SynchronizeGroupedState(
      tile, /*source=*/tile.first,
      model.GetTabGroupForTab(model.GetIndexOfTab(tile.first)));

  if (!tabs_are_adjacent) {
    tab_strip_model_adapter_.MakeTiledTabsAdjacent(tile);
  }

  for (auto& observer : observers_) {
    observer.OnTileTabs(tile);
  }
}

void SplitViewBrowserData::BreakTile(const tabs::TabHandle& tab) {
  if (auto iter = FindTile(tab); iter != tiles_.end()) {
    auto tile_to_break = *iter;
    for (auto& observer : observers_) {
      observer.OnWillBreakTile(tile_to_break);
    }

    auto index = tile_index_for_tab_[iter->first];
    tile_index_for_tab_.erase(iter->first);
    tile_index_for_tab_.erase(iter->second);
    for (auto& [t, tile_index] : tile_index_for_tab_) {
      if (tile_index > index) {
        tile_index--;
      }
    }

    tiles_.erase(iter);

    for (auto& observer : observers_) {
      observer.OnDidBreakTile(tile_to_break);
    }
  } else {
    NOTREACHED_IN_MIGRATION() << "Tile doesn't exist";
  }
}

std::vector<TabTile>::iterator SplitViewBrowserData::FindTile(
    const tabs::TabHandle& tab) {
  if (IsTabTiled(tab)) {
    return tiles_.begin() + tile_index_for_tab_[tab];
  }

  return tiles_.end();
}

std::vector<TabTile>::const_iterator SplitViewBrowserData::FindTile(
    const tabs::TabHandle& tab) const {
  return const_cast<SplitViewBrowserData*>(this)->FindTile(tab);
}

void SplitViewBrowserData::Transfer(SplitViewBrowserData* other,
                                    std::vector<TabTile> tiles) {
  for (const auto& tile : tiles) {
    other->TileTabs(tile);
  }
}

bool SplitViewBrowserData::IsTabTiled(const tabs::TabHandle& tab) const {
  return base::Contains(tile_index_for_tab_, tab);
}

void SplitViewBrowserData::SwapTabsInTile(const TabTile& tile) {
  auto iter = FindTile(tile.first);
  std::swap(iter->first, iter->second);

  for (auto& observer : observers_) {
    observer.OnSwapTabsInTile(*iter);
  }
}

std::optional<TabTile> SplitViewBrowserData::GetTile(
    const tabs::TabHandle& tab) const {
  auto iter = FindTile(tab);
  if (iter == tiles_.end()) {
    return std::nullopt;
  }
  return *iter;
}

void SplitViewBrowserData::SetSizeDelta(const tabs::TabHandle& tab,
                                        int size_delta) {
  auto iter = FindTile(tab);
  CHECK(iter != tiles_.end());

  iter->split_view_size_delta = size_delta;
}

int SplitViewBrowserData::GetSizeDelta(const tabs::TabHandle& tab) {
  auto iter = FindTile(tab);
  CHECK(iter != tiles_.end());
  return iter->split_view_size_delta;
}

void SplitViewBrowserData::AddObserver(SplitViewBrowserDataObserver* observer) {
  observers_.AddObserver(observer);
}

void SplitViewBrowserData::RemoveObserver(
    SplitViewBrowserDataObserver* observer) {
  observers_.RemoveObserver(observer);
}

SplitViewBrowserData::OnTabDragEndedClosure::OnTabDragEndedClosure() = default;

SplitViewBrowserData::OnTabDragEndedClosure::OnTabDragEndedClosure(
    SplitViewBrowserData* data,
    base::OnceClosure closure)
    : data_(data), closure_(std::move(closure)) {
  CHECK(data_);
}

SplitViewBrowserData::OnTabDragEndedClosure::OnTabDragEndedClosure(
    SplitViewBrowserData::OnTabDragEndedClosure&& other) noexcept {
  RunCurrentClosureIfNeededAndReplaceWith(std::move(other));
}

SplitViewBrowserData::OnTabDragEndedClosure&
SplitViewBrowserData::OnTabDragEndedClosure::operator=(
    SplitViewBrowserData::OnTabDragEndedClosure&& other) noexcept {
  RunCurrentClosureIfNeededAndReplaceWith(std::move(other));
  return *this;
}

void SplitViewBrowserData::OnTabDragEndedClosure::RunAndReset() {
  if (closure_) {
    closure_.RunAndReset();
  }
  data_ = nullptr;
}

void SplitViewBrowserData::OnTabDragEndedClosure::
    RunCurrentClosureIfNeededAndReplaceWith(OnTabDragEndedClosure&& other) {
  if (data_.get() == other.data_.get()) {
    // In case |this| and |other| are pointing at the same |data_|, just discard
    // the old one. This means we've got the callback from the same Browser
    // instance.
    if (closure_) {
      closure_.Release().Reset();
    }
  } else {
    // Target Browser was changed, so we need to run the callback for the old
    // target browser.
    if (closure_) {
      closure_.RunAndReset();
    }
  }

  data_ = nullptr;
  std::swap(data_, other.data_);
  std::swap(closure_, other.closure_);
}

SplitViewBrowserData::OnTabDragEndedClosure::~OnTabDragEndedClosure() = default;

SplitViewBrowserData::OnTabDragEndedClosure
SplitViewBrowserData::TabDragStarted() {
  tab_strip_model_adapter_.TabDragStarted();

  return OnTabDragEndedClosure(
      this, base::BindOnce(
                [](base::WeakPtr<SplitViewBrowserData> data) {
                  if (data) {
                    data->tab_strip_model_adapter_.TabDragEnded();
                  }
                },
                weak_ptr_factory_.GetWeakPtr()));
}

void SplitViewBrowserData::TabsWillBeAttachedToNewBrowser(
    const std::vector<tabs::TabHandle>& tabs) {
  DCHECK(tiles_to_be_attached_to_new_window_.empty());

  for (const auto& tab : tabs) {
    auto iter = FindTile(tab);
    if (iter == tiles_.end()) {
      continue;
    }

    tiles_to_be_attached_to_new_window_.push_back(*iter);
    // The tile in the |tile_| will be removed when they actually get detached
    // from the current tab strip model.
  }

  if (tiles_to_be_attached_to_new_window_.size() > 1) {
    base::ranges::sort(tiles_to_be_attached_to_new_window_);
    tiles_to_be_attached_to_new_window_.erase(
        base::ranges::unique(tiles_to_be_attached_to_new_window_,
                             [](auto& a, auto& b) {
                               return a.first == b.first &&
                                      a.second == b.second;
                             }),
        tiles_to_be_attached_to_new_window_.end());
  }
}

void SplitViewBrowserData::TabsAttachedToNewBrowser(Browser* browser) {
  Transfer(SplitViewBrowserData::FromBrowser(browser),
           std::move(tiles_to_be_attached_to_new_window_));
}

BROWSER_USER_DATA_KEY_IMPL(SplitViewBrowserData);
