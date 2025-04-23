/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_H_
#define BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_H_

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/tabs/tab_model.h"

class BrowserWindowInterface;
class SplitViewTabStripModelAdapter;
class SplitViewBrowserDataObserver;

// TabTile represents two tabs tied together like tile in tab strip UI.
// Split view shows tab tile's two tabs at once.
// Split view will put |first| tab first(left-side) and |second| one next.
// Two tabs in tile are located in adjacently and tab index of |first| is
// smaller thatn |second|.
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

// Handles tab tile operations such as create and break tab tile.
// Observe this to know about tab tile state changes.
class SplitViewBrowserData {
 public:
  explicit SplitViewBrowserData(
      BrowserWindowInterface* browser_window_interface);
  virtual ~SplitViewBrowserData();

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

  void AddObserver(SplitViewBrowserDataObserver* observer);
  void RemoveObserver(SplitViewBrowserDataObserver* observer);

  class OnTabDragEndedClosure {
   public:
    OnTabDragEndedClosure();
    OnTabDragEndedClosure(SplitViewBrowserData* data,
                          base::OnceClosure closure);
    OnTabDragEndedClosure(OnTabDragEndedClosure&& other) noexcept;
    OnTabDragEndedClosure& operator=(OnTabDragEndedClosure&& other) noexcept;
    ~OnTabDragEndedClosure();

    void RunAndReset();

   private:
    void RunCurrentClosureIfNeededAndReplaceWith(OnTabDragEndedClosure&& other);

    raw_ptr<SplitViewBrowserData> data_;

    base::ScopedClosureRunner closure_;
  };
  [[nodiscard]] OnTabDragEndedClosure TabDragStarted();

  void TabsWillBeAttachedToNewBrowser(const std::vector<tabs::TabHandle>& tabs);
  void TabsAttachedToNewBrowser(SplitViewBrowserData* target_data);

 private:
  friend class SplitViewBrowserDataBrowserTest;
  friend class SplitViewTabStripModelAdapterBrowserTest;

  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataBrowserTest,
                           BreakTile_WithNonExistingTabIsError);
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataBrowserTest,
                           TileTabs_WithAlreadyTiledTabIsError);
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataBrowserTest, FindTabTile);

  std::vector<TabTile>::iterator FindTabTile(const tabs::TabHandle& tab);
  std::vector<TabTile>::const_iterator FindTabTile(
      const tabs::TabHandle& tab) const;

  // When tabs are attached to another browser window and they are tiled tabs,
  // create tab tiles to that browser by using |tab_tiles|.
  void Transfer(SplitViewBrowserData* other, std::vector<TabTile> tab_tiles);

  std::unique_ptr<SplitViewTabStripModelAdapter> tab_strip_model_adapter_;

  std::vector<TabTile> tab_tiles_;
  std::vector<TabTile> tab_tiles_to_be_attached_to_new_window_;

  // As UI is likely to read more frequently than insert or delete, we cache
  // index for faster look up.
  base::flat_map<tabs::TabHandle, size_t> tab_tile_index_for_tab_;

  base::ObserverList<SplitViewBrowserDataObserver> observers_;

  raw_ptr<TabStripModel> tab_strip_model_for_testing_ = nullptr;

  base::WeakPtrFactory<SplitViewBrowserData> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_H_
