/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "brave/browser/ui/views/toolbar/brave_toolbar_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tab_search_bubble_host.h"
#include "chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h"
#include "chrome/browser/ui/views/tabs/shared/tab_strip_flat_edge_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget_utils.h"

// Tests for BraveHorizontalTabStripRegionView's combo-button trailing-edge
// repositioning.
class BraveHorizontalTabStripRegionViewBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveBrowserView* browser_view() {
    return BraveBrowserView::GetBrowserViewForBrowser(browser());
  }

  BraveToolbarView* toolbar_view() {
    return views::AsViewClass<BraveToolbarView>(browser_view()->toolbar());
  }

  BraveHorizontalTabStripRegionView* tab_strip_region() {
    views::View* tab_strip = browser_view()->horizontal_tab_strip_for_testing();
    return views::AsViewClass<BraveHorizontalTabStripRegionView>(
        tab_strip->parent());
  }
};

IN_PROC_BROWSER_TEST_F(BraveHorizontalTabStripRegionViewBrowserTest,
                       ComboButtonLayout) {
  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);

  TabStripComboButton* tab_strip_combo = region->GetComboButton();
  ASSERT_TRUE(tab_strip_combo)
      << "tab strip combo_button_ should exist when feature is enabled";

  TabSearchBubbleHost* bubble_host = browser_view()->GetTabSearchBubbleHost();
  ASSERT_TRUE(bubble_host) << "bubble host should exist always";
  EXPECT_FALSE(bubble_host->use_brave_vertical_tab());
  EXPECT_EQ(bubble_host->button(), tab_strip_combo->end_button());

  TabStripComboButton* toolbar_combo = toolbar_view()->combo_button();
  ASSERT_TRUE(toolbar_combo)
      << "toolbar combo_button_ should exist when feature is enabled";
  EXPECT_FALSE(toolbar_combo->GetVisible())
      << "toolbar combo button is hidden in horizontal tab";

  // Visible and placed at the trailing edge in horizontal mode.
  EXPECT_TRUE(tab_strip_combo->GetVisible());
  int expected_right = region->width();
#if BUILDFLAG(IS_MAC)
  expected_right -= 10;
#endif
  EXPECT_EQ(tab_strip_combo->bounds().right(), expected_right);
  EXPECT_EQ(tab_strip_combo->y(), region->GetInsets().top());

  // Last in z-order so it paints above the grab-handle space and stays
  // clickable.
  views::View::Views order = region->GetChildrenInZOrder();
  ASSERT_FALSE(order.empty());
  EXPECT_EQ(order.back(), tab_strip_combo);

  // Check button and bubble state after trun on vertical tab.
  // Hidden when vertical tabs are active (combo is repurposed away).
  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                                               true);
  RunScheduledLayouts();
  bubble_host = browser_view()->GetTabSearchBubbleHost();
  EXPECT_FALSE(tab_strip_combo->GetVisible())
      << "tabstrip combo is hidden in vertical tab";
  EXPECT_TRUE(toolbar_combo->GetVisible());
  EXPECT_TRUE(bubble_host->use_brave_vertical_tab());
  EXPECT_EQ(bubble_host->button(), toolbar_combo->end_button());

  // Check combo button position in toolbar.
  const auto back_button_index = toolbar_view()->GetIndexOf(
      browser_view()->toolbar_button_provider()->GetBackButton());
  const auto combo_button_index = toolbar_view()->GetIndexOf(toolbar_combo);
  EXPECT_EQ(*combo_button_index + 1, *back_button_index);
}
