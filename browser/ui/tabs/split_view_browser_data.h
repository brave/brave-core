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
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/tabs/tab_model.h"

class BrowserWindowInterface;
class SplitViewTabStripModelAdapter;
class SplitViewBrowserDataObserver;

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

class SplitViewBrowserData {
 public:
  explicit SplitViewBrowserData(
      BrowserWindowInterface* browser_window_interface);
  virtual ~SplitViewBrowserData();

  // When calling this, make sure that |tile.first| has a smaller model index
  // than |tile.second| be persistent across the all tab strip model operations.
  void TileTabs(const TabTile& tile);

  void BreakTile(const tabs::TabHandle& tab);

  bool IsTabTiled(const tabs::TabHandle& tab) const;

  void SwapTabsInTile(const TabTile& tile);

  std::optional<TabTile> GetTile(const tabs::TabHandle& tab) const;

  const std::vector<TabTile>& tiles() const { return tiles_; }

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
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataBrowserTest, FindTile);

  std::vector<TabTile>::iterator FindTile(const tabs::TabHandle& tab);
  std::vector<TabTile>::const_iterator FindTile(
      const tabs::TabHandle& tab) const;

  void Transfer(SplitViewBrowserData* other, std::vector<TabTile> tiles);

  std::unique_ptr<SplitViewTabStripModelAdapter> tab_strip_model_adapter_;

  std::vector<TabTile> tiles_;
  std::vector<TabTile> tiles_to_be_attached_to_new_window_;

  // As UI is likely to read more frequently than insert or delete, we cache
  // index for faster look up.
  base::flat_map<tabs::TabHandle, size_t> tile_index_for_tab_;

  base::ObserverList<SplitViewBrowserDataObserver> observers_;

  raw_ptr<TabStripModel> tab_strip_model_for_testing_ = nullptr;

  base::WeakPtrFactory<SplitViewBrowserData> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_H_
