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
#include "chrome/browser/ui/views/tabs/tab_strip.h"

class Tab;
class BraveTabStrip : public TabStrip {
  METADATA_HEADER(BraveTabStrip, TabStrip)
 public:
  explicit BraveTabStrip(std::unique_ptr<TabStripController> controller);
  ~BraveTabStrip() override;
  BraveTabStrip(const BraveTabStrip&) = delete;
  BraveTabStrip& operator=(const BraveTabStrip&) = delete;

  void EnterTabRenameModeAt(int index);

  bool ShouldShowPinnedTabsInGrid() const;

  // TabStrip:
  void ShowHover(Tab* tab, TabStyle::ShowHoverStyle style) override;
  void HideHover(Tab* tab, TabStyle::HideHoverStyle style) override;
  void UpdateHoverCard(Tab* tab, HoverCardUpdateType update_type) override;
  void MaybeStartDrag(TabSlotView* source,
                      const ui::LocatedEvent& event,
                      ui::ListSelectionModel original_selection) override;
  void AddedToWidget() override;
  std::optional<int> GetCustomBackgroundId(
      BrowserFrameActiveState active_state) const override;
  void SetCustomTitleForTab(
      Tab* tab,
      const std::optional<std::u16string>& title) override;
  bool ShouldAlwaysHideCloseButton() const override;
  bool IsVerticalTabsFloating() const override;
  bool CanPaintThrobberToLayer() const override;
  bool CanCloseTabViaMiddleButtonClick() const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollBarMode);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           BraveTabContainerSeparator);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollOffset);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           GetMaxScrollOffsetWithGroups);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ClipPathOnScrollOffset);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           LayoutAfterFirstTabCreation);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           ScrollBarDisabledWhenHorizontal);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           ScrollBarVisibilityWithManyTabs);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           ScrollBarBoundsWithPinnedTabs);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, ScrollBarThumbState);
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest,
                           RichAnimationIsDisabled);
  void UpdateOrientation();
  bool ShouldShowVerticalTabs() const;

  void OnAlwaysHideCloseButtonPrefChanged();

  TabContainer* GetTabContainerForTesting();

  BooleanPrefMember always_hide_close_button_;
  BooleanPrefMember middle_click_close_tab_enabled_;

  base::WeakPtrFactory<BraveTabStrip> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_BRAVE_TAB_STRIP_H_
