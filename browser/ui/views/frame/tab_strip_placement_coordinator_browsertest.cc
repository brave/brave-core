/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/tab_strip_placement_coordinator.h"

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test.h"
#include "ui/views/view.h"

class TabStripPlacementCoordinatorBrowserTest : public InProcessBrowserTest {
 protected:
  BraveBrowserView* brave_browser_view() {
    return BraveBrowserView::From(
        BrowserView::GetBrowserViewForBrowser(browser()));
  }

  TabStripPlacementCoordinator* coordinator() {
    return brave_browser_view()->tab_strip_placement_coordinator();
  }

  views::View* tab_strip_region_view() {
    return brave_browser_view()->tab_strip_view();
  }

  views::View* tab_strip_region_view_parent() {
    return tab_strip_region_view()->parent();
  }

  void SetVerticalTabsEnabled(bool enabled) {
    browser()->profile()->GetPrefs()->SetBoolean(
        brave_tabs::kVerticalTabsEnabled, enabled);
    RunScheduledLayouts();
  }
};

// Toggling the vertical-tabs pref reparents the tab strip into the vertical
// tab strip's region view, then back to its original parent.
IN_PROC_BROWSER_TEST_F(TabStripPlacementCoordinatorBrowserTest,
                       VerticalTabsTogglesParent) {
  views::View* const default_parent = tab_strip_region_view_parent();
  ASSERT_TRUE(default_parent);

  SetVerticalTabsEnabled(true);
  EXPECT_NE(default_parent, tab_strip_region_view_parent());
  EXPECT_TRUE(
      brave_browser_view()->vertical_tab_strip_container_view()->Contains(
          tab_strip_region_view()));

  SetVerticalTabsEnabled(false);
  EXPECT_EQ(default_parent, tab_strip_region_view_parent());
}

// When the browser is destroyed while in vertical tabs mode, the tabstrip is
// reparented back into its original placement such that no raw_ptrs in
// BrowserView are left dangling.
IN_PROC_BROWSER_TEST_F(TabStripPlacementCoordinatorBrowserTest,
                       VerticalTabsTeardownDoesNotDangleTabStrip) {
  browser()->profile()->GetPrefs()->SetBoolean(brave_tabs::kVerticalTabsEnabled,
                                               true);

  Browser* second = CreateBrowser(browser()->profile());
  ASSERT_TRUE(second);

  auto* second_view =
      BraveBrowserView::From(BrowserView::GetBrowserViewForBrowser(second));
  ASSERT_TRUE(second_view);
  ASSERT_TRUE(second_view->vertical_tab_strip_container_view()->Contains(
      second_view->tab_strip_view()));

  CloseBrowserSynchronously(second);
}
