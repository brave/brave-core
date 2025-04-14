/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/split_view/split_view_controller.h"

#include "brave/browser/ui/split_view/split_view_view.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

SplitViewController::SplitViewController(TabStripModel* tab_strip_model)
    : split_view_tab_tile_data_(tab_strip_model),
      tab_strip_model_(*tab_strip_model) {
  split_view_tab_tile_observation_.Observe(&split_view_tab_tile_data_);
}

SplitViewController::~SplitViewController() {
  split_view_tab_tile_observation_.Reset();
}

void SplitViewController::OnTileTabs(const TabTile& tile) {
  if (IsActiveWebContentsIncludedIn(tile) && view_) {
    view_->Update();
  }
}

void SplitViewController::OnDidBreakTile(const TabTile& tile) {
  if (IsActiveWebContentsIncludedIn(tile) && view_) {
    view_->Update();
  }
}

void SplitViewController::OnSwapTabsInTile(const TabTile& tile) {
  if (IsActiveWebContentsIncludedIn(tile) && view_) {
    view_->Update();
  }
}

bool SplitViewController::IsActiveWebContentsIncludedIn(
    const TabTile& tile) const {
  auto active_tab_handle = GetActiveTabHandle();
  return tile.first == active_tab_handle || tile.second == active_tab_handle;
}

bool SplitViewController::IsSplitViewActive() const {
  return split_view_tab_tile_data_.GetTile(GetActiveTabHandle()).has_value();
}

bool SplitViewController::IsOpenedFor(content::WebContents* contents) const {
  const int tab_index = tab_strip_model_->GetIndexOfWebContents(contents);
  if (tab_index == TabStripModel::kNoTab) {
    return false;
  }
  const auto tab_handle =
      tab_strip_model_->GetTabAtIndex(tab_index)->GetHandle();
  return split_view_tab_tile_data_.IsTabTiled(tab_handle);
}

bool SplitViewController::AreShowingTogether(
    const std::vector<content::WebContents*>& contents) {
  // We only support two views in split view.
  CHECK(contents.size() == 2);
  return GetTileFor(contents[0]) == GetTileFor(contents[1]);
}

bool SplitViewController::ShouldShowActiveWebContentsAtRight() const {
  CHECK(IsSplitViewActive());
  auto tile = GetActiveTabTile();
  return GetActiveTabHandle() == tile->second;
}

content::WebContents* SplitViewController::GetNonActiveWebContents() const {
  CHECK(IsSplitViewActive());
  auto tile = GetActiveTabTile();
  return tab_strip_model_->GetWebContentsAt(tab_strip_model_->GetIndexOfTab(
      ShouldShowActiveWebContentsAtRight() ? tile->first.Get()
                                           : tile->second.Get()));
}

std::optional<TabTile> SplitViewController::GetTileFor(
    content::WebContents* contents) const {
  auto tab_handle = GetTabHandleFor(contents);
  return split_view_tab_tile_data_.GetTile(tab_handle);
}

std::optional<TabTile> SplitViewController::GetActiveTabTile() const {
  auto tab_handle = GetActiveTabHandle();
  return split_view_tab_tile_data_.GetTile(tab_handle);
}

void SplitViewController::CacheSizeDeltaFor(content::WebContents* contents,
                                            int delta) {
  CHECK(GetTileFor(contents));
  auto tab_handle = GetTabHandleFor(contents);
  split_view_tab_tile_data_.SetSizeDelta(tab_handle, delta);
}

int SplitViewController::GetSizeDeltaFor(content::WebContents* contents) {
  CHECK(GetTileFor(contents));
  auto tab_handle = GetTabHandleFor(contents);
  return split_view_tab_tile_data_.GetSizeDelta(tab_handle);
}

tabs::TabHandle SplitViewController::GetActiveTabHandle() const {
  if (tab_strip_model_->empty()) {
    return {};
  }
  return tab_strip_model_->GetTabAtIndex(tab_strip_model_->active_index())
      ->GetHandle();
}

tabs::TabHandle SplitViewController::GetTabHandleFor(
    content::WebContents* contents) const {
  auto index = tab_strip_model_->GetIndexOfWebContents(contents);
  if (index == TabStripModel::kNoTab) {
    return {};
  }

  return tab_strip_model_->GetTabAtIndex(index)->GetHandle();
}
