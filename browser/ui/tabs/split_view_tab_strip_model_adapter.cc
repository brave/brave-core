/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/split_view_tab_strip_model_adapter.h"

#include "base/auto_reset.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/ranges/algorithm.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
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

void SplitViewTabStripModelAdapter::MakeTiledTabsAdjacent(const TabTile& tile,
                                                          bool move_right_tab) {
  auto index1 = model_->GetIndexOfTab(tile.first.Get());
  auto index2 = model_->GetIndexOfTab(tile.second.Get());

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

bool SplitViewTabStripModelAdapter::SynchronizeGroupedState(
    const TabTile& tile,
    const tabs::TabHandle& source,
    std::optional<tab_groups::TabGroupId> group) {
  DCHECK(!is_in_synch_grouped_state_);
  base::AutoReset<bool> resetter(&is_in_synch_grouped_state_, true);
  auto other_tab = tile.first == source ? tile.second : tile.first;
  auto other_tab_index = model_->GetIndexOfTab(other_tab.Get());
  auto tab_group_for_secondary_tab = model_->GetTabGroupForTab(other_tab_index);
  if (group == tab_group_for_secondary_tab) {
    return false;
  }

  auto tab_index = model_->GetIndexOfTab(other_tab.Get());
  if (group) {
    model_->AddToExistingGroup({tab_index}, group.value());
  } else {
    model_->RemoveFromGroup({tab_index});
  }

  MakeTiledTabsAdjacent(tile, true);
  return true;
}

bool SplitViewTabStripModelAdapter::SynchronizePinnedState(
    const TabTile& tile,
    const tabs::TabHandle& source) {
  auto tab1 = tile.first;
  auto tab2 = tile.second;
  if (tab1 != source) {
    std::swap(tab1, tab2);
    CHECK(tab1 == source);
  }

  const bool source_tab_is_pinned =
      model_->IsTabPinned(model_->GetIndexOfTab(source.Get()));
  if (source_tab_is_pinned ==
      model_->IsTabPinned(model_->GetIndexOfTab(tab2.Get()))) {
    return false;
  }

  model_->SetTabPinned(model_->GetIndexOfTab(tab2.Get()), source_tab_is_pinned);
  MakeTiledTabsAdjacent(tile, true);
  return true;
}

void SplitViewTabStripModelAdapter::TabDragEnded() {
  // Check if any tiles are separated after drag and drop session. Then break
  // the tiles.
  std::vector<TabTile> tiles_to_break;
  for (const auto& tile : split_view_browser_data_->tiles()) {
    int index1 = model_->GetIndexOfTab(tile.first.Get());
    int index2 = model_->GetIndexOfTab(tile.second.Get());
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
  for (const auto& tile : split_view_browser_data_->tiles()) {
    auto lower_index = model_->GetIndexOfTab(tile.first.Get());
    auto higher_index = model_->GetIndexOfTab(tile.second.Get());
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

              if (index != adapter->model_->GetIndexOfTab(tab.Get()))
                  [[unlikely]] {
                // Index changed. Cancel the move.
                return;
              }

              adapter->model_->MoveWebContentsAt(index, index + 1,
                                                 /*select_after_move*/ false);
            },
            weak_ptr_factory_.GetWeakPtr(),
            model_->GetTabAtIndex(index)->GetHandle(), index));
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
      model_->GetTabAtIndex(model_->GetIndexOfWebContents(move->contents))
          ->GetHandle();

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
    return model_->GetWebContentsAt(model_->GetIndexOfTab(tab.Get()));
  };

  // In case a tiled tab is removed, we need to remove the corresponding tile
  if (auto tab = model_->GetTabAtIndex(index)->GetHandle();
      split_view_browser_data_->IsTabTiled(tab)) {
    auto tile = *split_view_browser_data_->GetTile(tab);

    tiled_tabs_scheduled_to_be_removed_.emplace_back(
        get_web_contents_from_tab_handle(tile.first),
        get_web_contents_from_tab_handle(tile.second));

    split_view_browser_data_->BreakTile(tab);
  }
}

void SplitViewTabStripModelAdapter::TabPinnedStateChanged(
    TabStripModel* tab_strip_model,
    content::WebContents* contents,
    int index) {
  // In case a tiled tab is pinned or unpinned, we need to synchronize the other
  // tab together.
  auto changed_tab_handle = model_->GetTabAtIndex(index)->GetHandle();

  auto tile = split_view_browser_data_->GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  auto source_tab =
      model_->GetTabAtIndex(model_->GetIndexOfWebContents(contents))
          ->GetHandle();
  DCHECK(tile->first == source_tab || tile->second == source_tab);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<SplitViewTabStripModelAdapter> adapter,
                        TabTile tile, tabs::TabHandle source_tab) {
                       if (!adapter) {
                         return;
                       }

                       adapter->SynchronizePinnedState(tile, source_tab);
                     },
                     weak_ptr_factory_.GetWeakPtr(), *tile, source_tab));
}

void SplitViewTabStripModelAdapter::TabGroupedStateChanged(
    std::optional<tab_groups::TabGroupId> group,
    tabs::TabInterface* tab,
    int index) {
  if (!model_->ContainsIndex(index)) {
    return;
  }

  if (is_in_synch_grouped_state_) {
    return;
  }

  // In case a tiled tab is grouped or ungrouped, we need to synchronize the
  // other tab together.
  auto changed_tab_handle = model_->GetTabAtIndex(index)->GetHandle();
  auto tile = split_view_browser_data_->GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<SplitViewTabStripModelAdapter> adapter, TabTile tile,
             const tabs::TabHandle& source,
             std::optional<tab_groups::TabGroupId> group) {
            if (!adapter) {
              return;
            }

            adapter->SynchronizeGroupedState(tile, source, group);
          },
          weak_ptr_factory_.GetWeakPtr(), *tile, changed_tab_handle, group));
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
