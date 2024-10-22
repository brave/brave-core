/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_

#include <memory>
#include <optional>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

class Tab;
class BraveTabStrip : public TabStrip {
  METADATA_HEADER(BraveTabStrip, TabStrip)
 public:
  explicit BraveTabStrip(std::unique_ptr<TabStripController> controller);
  ~BraveTabStrip() override;
  BraveTabStrip(const BraveTabStrip&) = delete;
  BraveTabStrip& operator=(const BraveTabStrip&) = delete;

  bool IsVerticalTabsFloating() const;
  TabTiledState GetTiledStateForTab(int index) const;

  // TabStrip:
  void UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) override;
  void MaybeStartDrag(
      TabSlotView* source,
      const ui::LocatedEvent& event,
      const ui::ListSelectionModel& original_selection) override;
  void AddedToWidget() override;
  std::optional<int> GetCustomBackgroundId(
      BrowserFrameActiveState active_state) const override;
  bool IsTabTiled(const Tab* tab) const override;
  bool IsFirstTabInTile(const Tab* tab) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollBarVisibility);

  void UpdateTabContainer();
  bool ShouldShowVerticalTabs() const;

  std::optional<TabTile> GetTileForTab(const Tab* tab) const;

  TabContainer* GetTabContainerForTesting();

  // TabStrip overrides:
  bool ShouldDrawStrokes() const override;
  void Layout(PassKey) override;

  // Exposed for testing.
  static constexpr float kBraveMinimumContrastRatioForOutlines = 1.0816f;

  base::WeakPtrFactory<BraveTabStrip> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
