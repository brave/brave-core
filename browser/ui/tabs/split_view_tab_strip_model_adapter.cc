/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/tab_groups/tab_group_id.h"

SplitViewTabStripModelAdapter::SplitViewTabStripModelAdapter(
    SplitViewBrowserData& split_view_browser_data,
    TabStripModel* model)
    : split_view_browser_data_(split_view_browser_data), model_(*model) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

  model_->AddObserver(this);
}

SplitViewTabStripModelAdapter::~SplitViewTabStripModelAdapter() = default;

void SplitViewTabStripModelAdapter::MakeTiledTabsAdjacent(
    const SplitViewBrowserData::Tile& tile,
    bool move_right_tab) {
  auto [tab1, tab2] = tile;
  auto index1 = model_->GetIndexOfTab(tab1);
  auto index2 = model_->GetIndexOfTab(tab2);

  if (index1 + 1 == index2) {
    // Already adjacent
    return;
  }

  if (move_right_tab) {
    model_->MoveWebContentsAt(index2, index1 + 1, /*select_after_move*/ false);
  } else {
    model_->MoveWebContentsAt(index1, index2 - 1, /*select_after_move*/ false);
  }
}

void SplitViewTabStripModelAdapter::TabDragStarted() {
  if (split_view_browser_data_->tiles().empty() || is_in_tab_dragging()) {
    return;
  }

  is_in_tab_dragging_ = true;
}

void SplitViewTabStripModelAdapter::TabDragEnded() {
  // Check if any tiles are separated after drag and drop session. Then break
  // the tiles.
  std::vector<SplitViewBrowserData::Tile> tiles_to_break;
  for (const auto& tile : split_view_browser_data_->tiles()) {
    auto [tab1, tab2] = tile;
    int index1 = model_->GetIndexOfTab(tab1);
    int index2 = model_->GetIndexOfTab(tab2);
    if (index2 - index1 == 1) {
      return;
    }

    tiles_to_break.push_back(tile);
  }

  while (!tiles_to_break.empty()) {
    split_view_browser_data_->BreakTile(tiles_to_break.back().first);
    tiles_to_break.pop_back();
  }

  is_in_tab_dragging_ = false;
}

void SplitViewTabStripModelAdapter::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (split_view_browser_data_->tiles().empty()) {
    return;
  }

  switch (change.type()) {
    case TabStripModelChange::kInserted:
      OnTabInserted(change.GetInsert());
      break;
    case TabStripModelChange::kMoved:
      OnTabMoved(change.GetMove());
      break;
    case TabStripModelChange::kRemoved:
      OnTabRemoved(change.GetRemove());
      break;
    default:
      break;
  }
}

void SplitViewTabStripModelAdapter::OnTabInserted(
    const TabStripModelChange::Insert* insert) {
  // When tabs are inserted between tiles, we'll move it after the tile.
  // This can happen when the inserted tabs were created from tile.first.

  // Indices of tabs are at the time of insertion, so we need to adjust them.
  std::vector<int> inserted_indices;
  for (const auto& contents_with_index : insert->contents) {
    for (auto& already_inserted_index : inserted_indices) {
      if (already_inserted_index >= contents_with_index.index) {
        ++already_inserted_index;
      }
    }

    inserted_indices.push_back(contents_with_index.index);
  }

  std::vector<int> indices_to_be_moved;
  for (const auto& [tab1, tab2] : split_view_browser_data_->tiles()) {
    auto lower_index = model_->GetIndexOfTab(tab1);
    auto higher_index = model_->GetIndexOfTab(tab2);
    CHECK_LT(lower_index, higher_index);

    for (auto inserted_index : inserted_indices) {
      if (lower_index < inserted_index && inserted_index < higher_index) {
        indices_to_be_moved.push_back(inserted_index);
        break;
      }
    }
  }

  for (auto index : indices_to_be_moved) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](base::WeakPtr<SplitViewTabStripModelAdapter> adapter,
               tabs::TabHandle tab, int index) {
              if (!adapter) {
                return;
              }

              if (UNLIKELY(index != adapter->model_->GetIndexOfTab(tab))) {
                // Index changed. Cancel the move.
                return;
              }

              adapter->model_->MoveWebContentsAt(index, index + 1,
                                                 /*select_after_move*/ false);
            },
            weak_ptr_factory_.GetWeakPtr(), model_->GetTabHandleAt(index),
            index));
  }

  // TODO(sko) There're a few more things to consider
  // * When tabs are inserted from other window.
  // * When tabs are restored from cached state.
  // * When tabs are restored from the session restore(e.g. start up)
}

void SplitViewTabStripModelAdapter::OnTabMoved(
    const TabStripModelChange::Move* move) {
  // In case a tiled tab is moved, we need to move the corresponding tile
  // together
  auto tab_handle =
      model_->GetTabHandleAt(model_->GetIndexOfWebContents(move->contents));

  auto tile = split_view_browser_data_->GetTile(tab_handle);
  if (!tile) {
    return;
  }

  const bool move_right_tab = tile->first == tab_handle;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&SplitViewTabStripModelAdapter::MakeTiledTabsAdjacent,
                     weak_ptr_factory_.GetWeakPtr(), *tile, move_right_tab));
}

void SplitViewTabStripModelAdapter::OnTabWillBeRemoved(
    content::WebContents* contents,
    int index) {
  auto get_web_contents_from_tab_handle = [this](tabs::TabHandle tab) {
    return model_->GetWebContentsAt(model_->GetIndexOfTab(tab));
  };

  // In case a tiled tab is removed, we need to remove the corresponding tile
  if (auto tab = model_->GetTabHandleAt(index);
      split_view_browser_data_->IsTabTiled(tab)) {
    auto [tab1, tab2] = *split_view_browser_data_->GetTile(tab);

    tiled_tabs_scheduled_to_be_removed_.emplace_back(
        get_web_contents_from_tab_handle(tab1),
        get_web_contents_from_tab_handle(tab2));

    split_view_browser_data_->BreakTile(tab);
  }
}

void SplitViewTabStripModelAdapter::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    content::WebContents* contents,
    int index) {
  // In case a tiled tab is pinned or unpinned, we need to synchronize the other
  // tab together.
  auto changed_tab_handle = model_->GetTabHandleAt(index);

  auto tile = split_view_browser_data_->GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  auto [tab1, other_tab] = *tile;
  if (tab1 != changed_tab_handle) {
    std::swap(tab1, other_tab);
    CHECK(tab1 == changed_tab_handle);
  }

  if (model_->IsTabPinned(index) ==
      model_->IsTabPinned(model_->GetIndexOfTab(other_tab))) {
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<SplitViewTabStripModelAdapter> adapter,
                        SplitViewBrowserData::Tile tile, tabs::TabHandle tab,
                        bool pinned) {
                       if (!adapter) {
                         return;
                       }

                       adapter->model_->SetTabPinned(
                           adapter->model_->GetIndexOfTab(tab), pinned);
                       adapter->MakeTiledTabsAdjacent(tile, true);
                     },
                     weak_ptr_factory_.GetWeakPtr(), *tile, other_tab,
                     model_->IsTabPinned(index)));
}

void SplitViewTabStripModelAdapter::TabGroupedStateChanged(
    std::optional<tab_groups::TabGroupId> group,
    content::WebContents* contents,
    int index) {
  // In case a tiled tab is grouped or ungrouped, we need to synchronize the
  // other tab together.
  auto changed_tab_handle = model_->GetTabHandleAt(index);
  auto tile = split_view_browser_data_->GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  auto [tab1, other_tab] = *tile;
  if (tab1 != changed_tab_handle) {
    std::swap(tab1, other_tab);
    CHECK(tab1 == changed_tab_handle);
  }

  auto other_tab_index = model_->GetIndexOfTab(other_tab);
  auto tab_group_for_secondary_tab = model_->GetTabGroupForTab(other_tab_index);
  if (group == tab_group_for_secondary_tab) {
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<SplitViewTabStripModelAdapter> adapter,
             std::optional<tab_groups::TabGroupId> group_id,
             SplitViewBrowserData::Tile tile, tabs::TabHandle tab_to_move) {
            if (!adapter) {
              return;
            }

            auto tab_index = adapter->model_->GetIndexOfTab(tab_to_move);
            if (group_id) {
              adapter->model_->AddToExistingGroup({tab_index},
                                                  group_id.value());
            } else {
              adapter->model_->RemoveFromGroup({tab_index});
            }

            adapter->MakeTiledTabsAdjacent(tile, true);
          },
          weak_ptr_factory_.GetWeakPtr(), group, *tile, other_tab));
}

void SplitViewTabStripModelAdapter::OnTabRemoved(
    const TabStripModelChange::Remove* remove) {
  for (auto& removed_tabs : remove->contents) {
    if (removed_tabs.remove_reason !=
        TabStripModelChange::RemoveReason::kDeleted) {
      // In case the remove_reason is kCached or kInsertedIntoOtherTabStrip,
      // the data should remain so that we can re-tile the tabs.
      continue;
    }

    if (removed_tabs.remove_reason ==
        TabStripModelChange::RemoveReason::kDeleted) {
      auto iter = base::ranges::find_if(
          tiled_tabs_scheduled_to_be_removed_, [](const auto& contentses) {
            return !contentses.first || !contentses.second;
          });
      if (iter != tiled_tabs_scheduled_to_be_removed_.end()) {
        tiled_tabs_scheduled_to_be_removed_.erase(iter);
      }
      continue;
    }
  }
}
