/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/brave_tab_strip_region_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/frame/horizontal_tab_strip_region_view.h"
#include "chrome/browser/ui/views/tabs/shared/tab_strip_combo_button.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/view_utils.h"
#include "ui/views/widget/widget_utils.h"

// Tests for BraveHorizontalTabStripRegionView's combo-button trailing-edge
// repositioning. The kHorizontalTabStripComboButton feature is disabled by
// default in Brave, so it is enabled in the fixture constructor so the button
// is created before the browser window is built.
class BraveHorizontalTabStripRegionViewBrowserTest
    : public InProcessBrowserTest {
 public:
  BraveHorizontalTabStripRegionViewBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        tabs::kHorizontalTabStripComboButton);
  }

  BraveBrowserView* browser_view() {
    return static_cast<BraveBrowserView*>(browser()->window());
  }

  BraveHorizontalTabStripRegionView* tab_strip_region() {
    views::View* tab_strip = browser_view()->horizontal_tab_strip_for_testing();
    return views::AsViewClass<BraveHorizontalTabStripRegionView>(
        tab_strip->parent());
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(BraveHorizontalTabStripRegionViewBrowserTest,
                       ComboButtonLayout) {
  BraveHorizontalTabStripRegionView* region = tab_strip_region();
  ASSERT_TRUE(region);

  TabStripComboButton* combo = region->GetComboButton();
  ASSERT_TRUE(combo) << "combo_button_ should exist when feature is enabled";

  // Visible and placed at the trailing edge in horizontal mode.
  EXPECT_TRUE(combo->GetVisible());
  int expected_right = region->width();
#if BUILDFLAG(IS_MAC)
  expected_right -= 10;
#endif
  EXPECT_EQ(combo->bounds().right(), expected_right);
  EXPECT_EQ(combo->y(), region->GetInsets().top());

  // Last in z-order so it paints above the grab-handle space and stays
  // clickable.
  views::View::Views order = region->GetChildrenInZOrder();
  ASSERT_FALSE(order.empty());
  EXPECT_EQ(order.back(), combo);

  // Hidden when vertical tabs are active (combo is repurposed away).
  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                                               true);
  RunScheduledLayouts();
  EXPECT_FALSE(combo->GetVisible());
}
