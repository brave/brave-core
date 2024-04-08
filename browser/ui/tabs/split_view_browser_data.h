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
#include "chrome/browser/ui/browser_user_data.h"
#include "chrome/browser/ui/tabs/tab_model.h"

class SplitViewTabStripModelAdapter;
class SplitViewBrowserDataObserver;

class SplitViewBrowserData : public BrowserUserData<SplitViewBrowserData> {
 public:
  using Tile = std::pair<tabs::TabHandle, tabs::TabHandle>;

  ~SplitViewBrowserData() override;

  // When calling this, make sure that |tile.first| has a smaller model index
  // than |tile.second| be persistent across the all tab strip model operations.
  void TileTabs(const Tile& tile);

  void BreakTile(const tabs::TabHandle& tab);

  bool IsTabTiled(const tabs::TabHandle& tab) const;

  std::optional<Tile> GetTile(const tabs::TabHandle& tab) const;

  const std::vector<Tile>& tiles() const { return tiles_; }

  void AddObserver(SplitViewBrowserDataObserver* observer);
  void RemoveObserver(SplitViewBrowserDataObserver* observer);

  class OnTabDragEndedClosure {
   public:
    OnTabDragEndedClosure();
    OnTabDragEndedClosure(base::WeakPtr<SplitViewBrowserData> data,
                          base::OnceClosure closure);
    OnTabDragEndedClosure(OnTabDragEndedClosure&& other) noexcept;
    OnTabDragEndedClosure& operator=(OnTabDragEndedClosure&& other) noexcept;
    ~OnTabDragEndedClosure();

    void RunAndReset();

   private:
    void RunCurrentClosureIfNeededAndReplaceWith(OnTabDragEndedClosure&& other);

    base::WeakPtr<SplitViewBrowserData> data_;

    base::ScopedClosureRunner closure_;
  };
  [[nodiscard]] OnTabDragEndedClosure TabDragStarted();

 private:
  friend BrowserUserData;
  friend class SplitViewBrowserDataUnitTest;
  friend class SplitViewTabStripModelAdapterUnitTest;

  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataUnitTest,
                           BreakTile_WithNonExistingTabIsError);
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataUnitTest,
                           TileTabs_WithAlreadyTiledTabIsError);
  FRIEND_TEST_ALL_PREFIXES(SplitViewBrowserDataUnitTest, FindTile);

  explicit SplitViewBrowserData(Browser* browser);

  std::vector<Tile>::const_iterator FindTile(const tabs::TabHandle& tab);
  std::vector<Tile>::const_iterator FindTile(const tabs::TabHandle& tab) const;

  std::unique_ptr<SplitViewTabStripModelAdapter> tab_strip_model_adapter_;

  std::vector<Tile> tiles_;

  // As UI is likely to read more frequently than insert or delete, we cache
  // index for faster look up.
  base::flat_map<tabs::TabHandle, size_t> tile_index_for_tab_;

  base::ObserverList<SplitViewBrowserDataObserver> observers_;

  bool is_testing_ = false;

  base::WeakPtrFactory<SplitViewBrowserData> weak_ptr_factory_{this};

  BROWSER_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_UI_TABS_SPLIT_VIEW_BROWSER_DATA_H_
