/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_TAB_TILE_MODEL_H_
#define BRAVE_BROWSER_UI_TABS_TAB_TILE_MODEL_H_

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

class BrowserWindowInterface;
class TabTileModelObserver;

// TabTile represents two tabs tied together like tile in tab strip UI.
// As two tabs in a tab tile are located in adjacently, |first| tab is placed
// first in a tab strip and |second| is next. So, |second| tab's tab index is 1
// bigger than |first| one.
struct TabTile {
  tabs::TabHandle first;
  tabs::TabHandle second;

  // A absolute value means that the split view for |first| and |second|
  // should be resized by in pixel. When it's 0, the ratio between |first| and
  // |second| would be 0.5.
  int split_view_size_delta = 0;

  auto operator<=>(const TabTile& other) const noexcept {
    return std::tie(first, second) <=> std::tie(other.first, other.second);
  }
  bool operator==(const TabTile& other) const noexcept {
    return std::tie(first, second) == std::tie(other.first, other.second);
  }
};

// Represents tab tile state of current browser window.
// Client can ask tile create/break or swap tab's position in a tab tile.
// Observe this model to know about each tab tile state changes.
class TabTileModel : public TabStripModelObserver {
 public:
  explicit TabTileModel(BrowserWindowInterface* browser_window_interface);
  ~TabTileModel() override;

  // When calling this, make sure that |tile.first| has a smaller model index
  // than |tile.second| be persistent across the all tab strip model operations.
  void TileTabs(const TabTile& tab_tile);

  // Break tab tile that includes |tab|.
  void BreakTile(const tabs::TabHandle& tab);

  // true when |tab| is included existing tab tile.
  bool IsTabTiled(const tabs::TabHandle& tab) const;

  // Swap first and second tabs in |tile|.
  void SwapTabsInTile(const TabTile& tab_tile);

  std::optional<TabTile> GetTile(const tabs::TabHandle& tab) const;

  const std::vector<TabTile>& tab_tiles() const { return tab_tiles_; }

  void SetSizeDelta(const tabs::TabHandle& tab, int size_delta);
  int GetSizeDelta(const tabs::TabHandle& tab);

  void AddObserver(TabTileModelObserver* observer);
  void RemoveObserver(TabTileModelObserver* observer);

  class OnTabDragEndedClosure {
   public:
    OnTabDragEndedClosure();
    OnTabDragEndedClosure(TabTileModel* data, base::OnceClosure closure);
    OnTabDragEndedClosure(OnTabDragEndedClosure&& other) noexcept;
    OnTabDragEndedClosure& operator=(OnTabDragEndedClosure&& other) noexcept;
    ~OnTabDragEndedClosure();

    void RunAndReset();

   private:
    void RunCurrentClosureIfNeededAndReplaceWith(OnTabDragEndedClosure&& other);

    raw_ptr<TabTileModel> data_;

    base::ScopedClosureRunner closure_;
  };
  [[nodiscard]] OnTabDragEndedClosure TabDragStarted();

  void TabsWillBeAttachedToNewBrowser(const std::vector<tabs::TabHandle>& tabs);
  void TabsAttachedToNewBrowser(TabTileModel* target_data);

 private:
  friend class TabTileModelBrowserTest;

  FRIEND_TEST_ALL_PREFIXES(TabTileModelBrowserTest,
                           BreakTile_WithNonExistingTabIsError);
  FRIEND_TEST_ALL_PREFIXES(TabTileModelBrowserTest,
                           TileTabs_WithAlreadyTiledTabIsError);
  FRIEND_TEST_ALL_PREFIXES(TabTileModelBrowserTest, FindTabTile);

  std::vector<TabTile>::iterator FindTabTile(const tabs::TabHandle& tab);
  std::vector<TabTile>::const_iterator FindTabTile(
      const tabs::TabHandle& tab) const;

  // When tabs are attached to another browser window and they are tiled tabs,
  // create tab tiles to that browser by using |tab_tiles|.
  void Transfer(TabTileModel* other, std::vector<TabTile> tab_tiles);

  void MakeTiledTabsAdjacent(const TabTile& tile, bool move_right_tab = true);
  bool SynchronizeGroupedState(const TabTile& tile,
                               const tabs::TabHandle& source,
                               std::optional<tab_groups::TabGroupId> group);
  bool SynchronizePinnedState(const TabTile& tile,
                              const tabs::TabHandle& source);

  void TabDragEnded();
  void OnTabInserted(const TabStripModelChange::Insert* insert);
  void OnTabMoved(const TabStripModelChange::Move* move);
  void OnTabRemoved(const TabStripModelChange::Remove* remove);

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void OnTabWillBeRemoved(content::WebContents* contents, int index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabGroupedStateChanged(TabStripModel* tab_strip_model,
                              std::optional<tab_groups::TabGroupId> old_group,
                              std::optional<tab_groups::TabGroupId> new_group,
                              tabs::TabInterface* tab,
                              int index) override;

  // Filled up in OnTabWillBeRemoved() and revisited from OnTabRemoved().
  // These are in pending state until we can decide what to do with them in
  // OnTabRemoved() by looking into the reason.
  std::vector<std::pair<content::WebContents*, content::WebContents*>>
      tiled_tabs_scheduled_to_be_removed_;

  raw_ref<TabStripModel> model_;

  bool is_in_tab_dragging_ = false;

  bool is_in_synch_grouped_state_ = false;

  std::vector<TabTile> tab_tiles_;
  std::vector<TabTile> tab_tiles_to_be_attached_to_new_window_;

  // As UI is likely to find TabHandle's TabTile in |tab_tiles_| more frequently
  // than insert or delete to it, we cache TabHandle's index in |tab_tiles_| for
  // faster look up.
  base::flat_map<tabs::TabHandle, size_t> tab_tile_index_for_tab_;

  base::ObserverList<TabTileModelObserver> observers_;

  raw_ptr<TabStripModel> tab_strip_model_for_testing_ = nullptr;

  base::WeakPtrFactory<TabTileModel> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_TAB_TILE_MODEL_H_
