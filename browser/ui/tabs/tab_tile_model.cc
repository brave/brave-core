/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/tab_tile_model.h"

#include <algorithm>

#include "base/auto_reset.h"
#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/notreached.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/misc_metrics/process_misc_metrics.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/tab_tile_model_observer.h"
#include "brave/components/misc_metrics/split_view_metrics.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "components/tab_groups/tab_group_id.h"

TabTileModel::TabTileModel(BrowserWindowInterface* browser_window_interface)
    : model_(*browser_window_interface->GetTabStripModel()) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveSplitView));

  model_->AddObserver(this);
}

TabTileModel::~TabTileModel() {
  // base::ObserverList is safe to be mutated during iteration.
  for (auto& observer : observers_) {
    observer.OnWillDeleteTabTileModel();
  }
}

void TabTileModel::TileTabs(const TabTile& tab_tile) {
  CHECK(!IsTabTiled(tab_tile.first));
  CHECK(!IsTabTiled(tab_tile.second));

  CHECK_LT(model_->GetIndexOfTab(tab_tile.first.Get()),
           model_->GetIndexOfTab(tab_tile.second.Get()));

  auto* process_misc_metrics = g_brave_browser_process->process_misc_metrics();
  if (process_misc_metrics) {
    process_misc_metrics->split_view_metrics()->ReportSplitViewUsage();
  }

  tab_tiles_.push_back(tab_tile);

  tab_tile_index_for_tab_[tab_tile.first] = tab_tiles_.size() - 1;
  tab_tile_index_for_tab_[tab_tile.second] = tab_tiles_.size() - 1;

  bool tabs_are_adjacent = SynchronizePinnedState(tab_tile, tab_tile.first);
  tabs_are_adjacent |= SynchronizeGroupedState(
      tab_tile, /*source=*/tab_tile.first,
      model_->GetTabGroupForTab(model_->GetIndexOfTab(tab_tile.first.Get())));

  if (!tabs_are_adjacent) {
    MakeTiledTabsAdjacent(tab_tile);
  }

  for (auto& observer : observers_) {
    observer.OnTileTabs(tab_tile);
  }
}

void TabTileModel::BreakTile(const tabs::TabHandle& tab) {
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

std::vector<TabTile>::iterator TabTileModel::FindTabTile(
    const tabs::TabHandle& tab) {
  if (IsTabTiled(tab)) {
    return tab_tiles_.begin() + tab_tile_index_for_tab_[tab];
  }

  return tab_tiles_.end();
}

std::vector<TabTile>::const_iterator TabTileModel::FindTabTile(
    const tabs::TabHandle& tab) const {
  return const_cast<TabTileModel*>(this)->FindTabTile(tab);
}

void TabTileModel::Transfer(TabTileModel* other,
                            std::vector<TabTile> tab_tiles) {
  for (const auto& tab_tile : tab_tiles) {
    other->TileTabs(tab_tile);
  }
}

bool TabTileModel::IsTabTiled(const tabs::TabHandle& tab) const {
  return tab_tile_index_for_tab_.contains(tab);
}

void TabTileModel::SwapTabsInTile(const TabTile& tab_tile) {
  auto iter = FindTabTile(tab_tile.first);
  std::swap(iter->first, iter->second);

  for (auto& observer : observers_) {
    observer.OnSwapTabsInTile(*iter);
  }
}

std::optional<TabTile> TabTileModel::GetTile(const tabs::TabHandle& tab) const {
  auto iter = FindTabTile(tab);
  if (iter == tab_tiles_.end()) {
    return std::nullopt;
  }
  return *iter;
}

void TabTileModel::SetSizeDelta(const tabs::TabHandle& tab, int size_delta) {
  auto iter = FindTabTile(tab);
  CHECK(iter != tab_tiles_.end());

  iter->split_view_size_delta = size_delta;
}

int TabTileModel::GetSizeDelta(const tabs::TabHandle& tab) {
  auto iter = FindTabTile(tab);
  CHECK(iter != tab_tiles_.end());
  return iter->split_view_size_delta;
}

void TabTileModel::AddObserver(TabTileModelObserver* observer) {
  observers_.AddObserver(observer);
}

void TabTileModel::RemoveObserver(TabTileModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

TabTileModel::OnTabDragEndedClosure::OnTabDragEndedClosure() = default;

TabTileModel::OnTabDragEndedClosure::OnTabDragEndedClosure(
    TabTileModel* data,
    base::OnceClosure closure)
    : data_(data), closure_(std::move(closure)) {
  CHECK(data_);
}

TabTileModel::OnTabDragEndedClosure::OnTabDragEndedClosure(
    TabTileModel::OnTabDragEndedClosure&& other) noexcept {
  RunCurrentClosureIfNeededAndReplaceWith(std::move(other));
}

TabTileModel::OnTabDragEndedClosure&
TabTileModel::OnTabDragEndedClosure::operator=(
    TabTileModel::OnTabDragEndedClosure&& other) noexcept {
  RunCurrentClosureIfNeededAndReplaceWith(std::move(other));
  return *this;
}

void TabTileModel::OnTabDragEndedClosure::RunAndReset() {
  if (closure_) {
    closure_.RunAndReset();
  }
  data_ = nullptr;
}

void TabTileModel::OnTabDragEndedClosure::
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

TabTileModel::OnTabDragEndedClosure::~OnTabDragEndedClosure() = default;

TabTileModel::OnTabDragEndedClosure TabTileModel::TabDragStarted() {
  if (!tab_tiles().empty() && !is_in_tab_dragging_) {
    is_in_tab_dragging_ = true;
  }

  return OnTabDragEndedClosure(this, base::BindOnce(
                                         [](base::WeakPtr<TabTileModel> model) {
                                           if (model) {
                                             model->TabDragEnded();
                                           }
                                         },
                                         weak_ptr_factory_.GetWeakPtr()));
}

void TabTileModel::TabsWillBeAttachedToNewBrowser(
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

void TabTileModel::TabsAttachedToNewBrowser(TabTileModel* target_data) {
  Transfer(target_data, std::move(tab_tiles_to_be_attached_to_new_window_));
}

void TabTileModel::MakeTiledTabsAdjacent(const TabTile& tile,
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

bool TabTileModel::SynchronizeGroupedState(
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

bool TabTileModel::SynchronizePinnedState(const TabTile& tile,
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

void TabTileModel::TabDragEnded() {
  // Check if any tiles are separated after drag and drop session. Then break
  // the tiles.
  std::vector<TabTile> tiles_to_break;
  for (const auto& tile : tab_tiles()) {
    int index1 = model_->GetIndexOfTab(tile.first.Get());
    int index2 = model_->GetIndexOfTab(tile.second.Get());
    if (index2 - index1 == 1) {
      return;
    }

    tiles_to_break.push_back(tile);
  }

  while (!tiles_to_break.empty()) {
    BreakTile(tiles_to_break.back().first);
    tiles_to_break.pop_back();
  }

  is_in_tab_dragging_ = false;
}

void TabTileModel::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  if (tab_tiles().empty()) {
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

void TabTileModel::OnTabInserted(const TabStripModelChange::Insert* insert) {
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
  for (const auto& tile : tab_tiles()) {
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
        FROM_HERE, base::BindOnce(
                       [](base::WeakPtr<TabTileModel> tab_tile_model,
                          tabs::TabHandle tab, int index) {
                         if (!tab_tile_model) {
                           return;
                         }

                         if (index != tab_tile_model->model_->GetIndexOfTab(
                                          tab.Get())) [[unlikely]] {
                           // Index changed. Cancel the move.
                           return;
                         }

                         tab_tile_model->model_->MoveWebContentsAt(
                             index, index + 1,
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

void TabTileModel::OnTabMoved(const TabStripModelChange::Move* move) {
  // In case a tiled tab is moved, we need to move the corresponding tile
  // together
  auto tab_handle =
      model_->GetTabAtIndex(model_->GetIndexOfWebContents(move->contents))
          ->GetHandle();

  auto tile = GetTile(tab_handle);
  if (!tile) {
    return;
  }

  const bool move_right_tab = tile->first == tab_handle;

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&TabTileModel::MakeTiledTabsAdjacent,
                     weak_ptr_factory_.GetWeakPtr(), *tile, move_right_tab));
}

void TabTileModel::OnTabWillBeRemoved(content::WebContents* contents,
                                      int index) {
  auto get_web_contents_from_tab_handle = [this](tabs::TabHandle tab) {
    return model_->GetWebContentsAt(model_->GetIndexOfTab(tab.Get()));
  };

  // In case a tiled tab is removed, we need to remove the corresponding tile
  if (auto tab = model_->GetTabAtIndex(index)->GetHandle(); IsTabTiled(tab)) {
    auto tile = *GetTile(tab);

    tiled_tabs_scheduled_to_be_removed_.emplace_back(
        get_web_contents_from_tab_handle(tile.first),
        get_web_contents_from_tab_handle(tile.second));

    BreakTile(tab);
  }
}

void TabTileModel::TabPinnedStateChanged(TabStripModel* tab_strip_model,
                                         content::WebContents* contents,
                                         int index) {
  // In case a tiled tab is pinned or unpinned, we need to synchronize the other
  // tab together.
  auto changed_tab_handle = model_->GetTabAtIndex(index)->GetHandle();

  auto tile = GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  auto source_tab =
      model_->GetTabAtIndex(model_->GetIndexOfWebContents(contents))
          ->GetHandle();
  DCHECK(tile->first == source_tab || tile->second == source_tab);

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TabTileModel> tab_tile_model,
                        TabTile tile, tabs::TabHandle source_tab) {
                       if (!tab_tile_model) {
                         return;
                       }

                       tab_tile_model->SynchronizePinnedState(tile, source_tab);
                     },
                     weak_ptr_factory_.GetWeakPtr(), *tile, source_tab));
}

void TabTileModel::TabGroupedStateChanged(
    TabStripModel* tab_strip_model,
    std::optional<tab_groups::TabGroupId> old_group,
    std::optional<tab_groups::TabGroupId> new_group,
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
  auto tile = GetTile(changed_tab_handle);
  if (!tile) {
    return;
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(
                     [](base::WeakPtr<TabTileModel> tab_tile_model,
                        TabTile tile, const tabs::TabHandle& source,
                        std::optional<tab_groups::TabGroupId> group) {
                       if (!tab_tile_model) {
                         return;
                       }

                       tab_tile_model->SynchronizeGroupedState(tile, source,
                                                               group);
                     },
                     weak_ptr_factory_.GetWeakPtr(), *tile, changed_tab_handle,
                     new_group));
}

void TabTileModel::OnTabRemoved(const TabStripModelChange::Remove* remove) {
  for (auto& removed_tabs : remove->contents) {
    if (removed_tabs.remove_reason !=
        TabStripModelChange::RemoveReason::kDeleted) {
      // In case the remove_reason is kCached or kInsertedIntoOtherTabStrip,
      // the data should remain so that we can re-tile the tabs.
      continue;
    }

    if (removed_tabs.remove_reason ==
        TabStripModelChange::RemoveReason::kDeleted) {
      auto iter = std::ranges::find_if(
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
