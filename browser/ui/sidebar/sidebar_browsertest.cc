/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/brave_browser.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/components/sidebar/features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"

namespace sidebar {

class SidebarBrowserTest : public InProcessBrowserTest,
                           public SidebarModel::Observer {
 public:
  SidebarBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(kSidebarFeature);
  }
  ~SidebarBrowserTest() override = default;

  BraveBrowser* brave_browser() {
    return static_cast<BraveBrowser*>(browser());
  }

  SidebarModel* model() { return controller()->model(); }

  SidebarController* controller() {
    return brave_browser()->sidebar_controller();
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(SidebarBrowserTest, BasicTest) {
  // Initially, active index is not set.
  EXPECT_EQ(-1, model()->active_index());

  // Check sidebar UI is initalized properly.
  EXPECT_TRUE(!!controller()->sidebar());

  // If current active tab is not NTP, we can add current url to sidebar.
  EXPECT_TRUE(CanAddCurrentActiveTabToSidebar(browser()));

  // If current active tab is NTP, we can't add current url to sidebar.
  ui_test_utils::NavigateToURL(browser(), GURL("brave://newtab/"));
  EXPECT_FALSE(CanAddCurrentActiveTabToSidebar(browser()));

  // Currently we have 4 default items.
  EXPECT_EQ(4UL, model()->GetAllSidebarItems().size());
  controller()->ActivateItemAt(0);
  EXPECT_EQ(0, model()->active_index());
  EXPECT_TRUE(controller()->IsActiveIndex(0));

  // Try to activate item at index 1.
  // Default item at index 1 opens in new tab. So, sidebar active index is not
  // changed. Still active index is 0.
  const auto item = model()->GetAllSidebarItems()[1];
  EXPECT_FALSE(item.open_in_panel);
  controller()->ActivateItemAt(1);
  EXPECT_EQ(0, model()->active_index());

  // Try to activate item at index 2.
  controller()->ActivateItemAt(2);
  EXPECT_EQ(2, model()->active_index());

  // Setting -1 means deactivate current active tab.
  controller()->ActivateItemAt(-1);
  EXPECT_EQ(-1, model()->active_index());

  controller()->ActivateItemAt(3);

  // Remove Item at index 0 change active index from 3 to 2.
  controller()->RemoveItemAt(0);
  EXPECT_EQ(3UL, model()->GetAllSidebarItems().size());
  EXPECT_EQ(2, model()->active_index());

  // Check |BrowserView::find_bar_host_view_| is the last child view.
  // If not, findbar dialog is not positioned properly.
  BrowserView* browser_view = BrowserView::GetBrowserViewForBrowser(browser());
  const size_t find_bar_host_view_index =
      browser_view->GetIndexOf(browser_view->find_bar_host_view());
  EXPECT_EQ(browser_view->children().size() - 1, find_bar_host_view_index);
}

}  // namespace sidebar
