/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_browser_data.h"

#include <algorithm>

#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/notreached.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data_observer.h"
#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"
#include "brave/components/misc_metrics/split_view_metrics.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

SplitViewBrowserData::SplitViewBrowserData(
    BrowserWindowInterface* browser_window_interface)
    : tab_strip_model_adapter_(std::make_unique<SplitViewTabStripModelAdapter>(
          *this,
          browser_window_interface->GetTabStripModel())) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));
}

SplitViewBrowserData::~SplitViewBrowserData() {
  // base::ObserverList is safe to be mutated during iteration.
  for (auto& observer : observers_) {
    observer.OnWillDeleteBrowserData();
  }
}

void SplitViewBrowserData::TileTabs(const TabTile& tab_tile) {
  CHECK(!IsTabTiled(tab_tile.first));
  CHECK(!IsTabTiled(tab_tile.second));

  auto& model = tab_strip_model_adapter_->tab_strip_model();
  CHECK_LT(model.GetIndexOfTab(tab_tile.first.Get()),
           model.GetIndexOfTab(tab_tile.second.Get()));

  auto* process_misc_metrics = g_brave_browser_process->process_misc_metrics();
  if (process_misc_metrics) {
    process_misc_metrics->split_view_metrics()->ReportSplitViewUsage();
  }

  tab_tiles_.push_back(tab_tile);

  tab_tile_index_for_tab_[tab_tile.first] = tab_tiles_.size() - 1;
  tab_tile_index_for_tab_[tab_tile.second] = tab_tiles_.size() - 1;

  bool tabs_are_adjacent = tab_strip_model_adapter_->SynchronizePinnedState(
      tab_tile, tab_tile.first);
  tabs_are_adjacent |= tab_strip_model_adapter_->SynchronizeGroupedState(
      tab_tile, /*source=*/tab_tile.first,
      model.GetTabGroupForTab(model.GetIndexOfTab(tab_tile.first.Get())));

  if (!tabs_are_adjacent) {
    tab_strip_model_adapter_->MakeTiledTabsAdjacent(tab_tile);
  }

  for (auto& observer : observers_) {
    observer.OnTileTabs(tab_tile);
  }
}

void SplitViewBrowserData::BreakTile(const tabs::TabHandle& tab) {
  if (auto iter = FindTabTile(tab); iter != tab_tiles_.end()) {
    auto tab_tile_to_break = *iter;
    for (auto& observer : observers_) {
      observer.OnWillBreakTile(tab_tile_to_break);
    }

    auto index = tab_tile_index_for_tab_[iter->first];
    tab_tile_index_for_tab_.erase(iter->first);
    tab_tile_index_for_tab_.erase(iter->second);
    for (auto& [t, tab_tile_index] : tab_tile_index_for_tab_) {
      if (tab_tile_index > index) {
        tab_tile_index--;
      }
    }

    tab_tiles_.erase(iter);

    for (auto& observer : observers_) {
      observer.OnDidBreakTile(tab_tile_to_break);
    }
  } else {
    NOTREACHED() << "Tried to break tile which doesn't exist";
  }
}

std::vector<TabTile>::iterator SplitViewBrowserData::FindTabTile(
    const tabs::TabHandle& tab) {
  if (IsTabTiled(tab)) {
    return tab_tiles_.begin() + tab_tile_index_for_tab_[tab];
  }

  return tab_tiles_.end();
}

std::vector<TabTile>::const_iterator SplitViewBrowserData::FindTabTile(
    const tabs::TabHandle& tab) const {
  return const_cast<SplitViewBrowserData*>(this)->FindTabTile(tab);
}

void SplitViewBrowserData::Transfer(SplitViewBrowserData* other,
                                    std::vector<TabTile> tab_tiles) {
  for (const auto& tab_tile : tab_tiles) {
    other->TileTabs(tab_tile);
  }
}

bool SplitViewBrowserData::IsTabTiled(const tabs::TabHandle& tab) const {
  return tab_tile_index_for_tab_.contains(tab);
}

void SplitViewBrowserData::SwapTabsInTile(const TabTile& tab_tile) {
  auto iter = FindTabTile(tab_tile.first);
  std::swap(iter->first, iter->second);

  for (auto& observer : observers_) {
    observer.OnSwapTabsInTile(*iter);
  }
}

std::optional<TabTile> SplitViewBrowserData::GetTile(
    const tabs::TabHandle& tab) const {
  auto iter = FindTabTile(tab);
  if (iter == tab_tiles_.end()) {
    return std::nullopt;
  }
  return *iter;
}

void SplitViewBrowserData::SetSizeDelta(const tabs::TabHandle& tab,
                                        int size_delta) {
  auto iter = FindTabTile(tab);
  CHECK(iter != tab_tiles_.end());

  iter->split_view_size_delta = size_delta;
}

int SplitViewBrowserData::GetSizeDelta(const tabs::TabHandle& tab) {
  auto iter = FindTabTile(tab);
  CHECK(iter != tab_tiles_.end());
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
  tab_strip_model_adapter_->TabDragStarted();

  return OnTabDragEndedClosure(
      this, base::BindOnce(
                [](base::WeakPtr<SplitViewBrowserData> data) {
                  if (data) {
                    data->tab_strip_model_adapter_->TabDragEnded();
                  }
                },
                weak_ptr_factory_.GetWeakPtr()));
}

void SplitViewBrowserData::TabsWillBeAttachedToNewBrowser(
    const std::vector<tabs::TabHandle>& tabs) {
  DCHECK(tab_tiles_to_be_attached_to_new_window_.empty());

  for (const auto& tab : tabs) {
    auto iter = FindTabTile(tab);
    if (iter == tab_tiles_.end()) {
      continue;
    }

    tab_tiles_to_be_attached_to_new_window_.push_back(*iter);
    // The tile in the |tab_tile_| will be removed when they actually get
    // detached from the current tab strip model.
  }

  if (tab_tiles_to_be_attached_to_new_window_.size() > 1) {
    std::ranges::sort(tab_tiles_to_be_attached_to_new_window_);
    auto to_remove =
        std::ranges::unique(tab_tiles_to_be_attached_to_new_window_);
    tab_tiles_to_be_attached_to_new_window_.erase(to_remove.begin(),
                                                  to_remove.end());
  }
}

void SplitViewBrowserData::TabsAttachedToNewBrowser(
    SplitViewBrowserData* target_data) {
  Transfer(target_data, std::move(tab_tiles_to_be_attached_to_new_window_));
}
