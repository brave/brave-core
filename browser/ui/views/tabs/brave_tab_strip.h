/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_

#include <memory>
#include <optional>

#include "base/gtest_prod_util.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

class Tab;
class BraveTabStrip : public TabStrip {
 public:
  METADATA_HEADER(BraveTabStrip);

  explicit BraveTabStrip(std::unique_ptr<TabStripController> controller);
  ~BraveTabStrip() override;
  BraveTabStrip(const BraveTabStrip&) = delete;
  BraveTabStrip& operator=(const BraveTabStrip&) = delete;

  bool IsVerticalTabsFloating() const;

  // TabStrip:
  void UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) override;
  void MaybeStartDrag(
      TabSlotView* source,
      const ui::LocatedEvent& event,
      const ui::ListSelectionModel& original_selection) override;
  void AddedToWidget() override;
  std::optional<int> GetCustomBackgroundId(
      BrowserFrameActiveState active_state) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollBarVisibility);

  void UpdateTabContainer();
  bool ShouldShowVerticalTabs() const;

  TabContainer* GetTabContainerForTesting();

  // TabStrip overrides:
  SkColor GetTabSeparatorColor() const override;
  bool ShouldDrawStrokes() const override;
  void Layout() override;
  void OnPaintBackground(gfx::Canvas* canvas) override;

  // Exposed for testing.
  static constexpr float kBraveMinimumContrastRatioForOutlines = 1.0816f;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
