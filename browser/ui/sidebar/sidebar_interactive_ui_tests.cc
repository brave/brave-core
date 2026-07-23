/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "brave/browser/ui/sidebar/sidebar_browsertest_base.h"
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/interactive_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/interaction/interactive_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

namespace sidebar {

using SidebarInteractiveUITest =
    InteractiveBrowserTestMixin<SidebarBrowserTest>;

#if BUILDFLAG(ENABLE_BRAVE_WALLET)

// This test relies on real window activation, which is only reliable in
// interactive UI tests (window focus is not available for the sharded
// browser_tests binary). It therefore lives here rather than in
// sidebar_browsertest.cc.
IN_PROC_BROWSER_TEST_F(SidebarInteractiveUITest, IterateBuiltInWebTypeTest) {
  // Click builtin wallet item and it's loaded at current active tab.
  const auto items = model()->GetAllSidebarItems();
  const auto wallet_item_iter =
      std::ranges::find(items, SidebarItem::BuiltInItemType::kWallet,
                        &SidebarItem::built_in_item_type);
  ASSERT_NE(wallet_item_iter, items.cend());
  const int wallet_item_index = std::distance(items.cbegin(), wallet_item_iter);
  auto wallet_item = model()->GetAllSidebarItems()[wallet_item_index];
  EXPECT_FALSE(controller()->DoesBrowserHaveOpenedTabForItem(wallet_item));
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_TRUE(controller()->DoesBrowserHaveOpenedTabForItem(wallet_item));
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            wallet_item.url.host());

  // Create NTP and click wallet item. Then wallet tab(index 0) is activated.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab.
  EXPECT_EQ(1, tab_model()->active_index());
  SimulateSidebarItemClickAt(wallet_item_index);
  // Wallet tab is active tab.
  EXPECT_EQ(0, tab_model()->active_index());
  EXPECT_EQ(tab_model()->GetWebContentsAt(0)->GetVisibleURL().host(),
            wallet_item.url.host());

  // Create NTP.
  ASSERT_TRUE(ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL("brave://newtab/"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP));
  // NTP is active tab and load wallet on it.
  EXPECT_EQ(2, tab_model()->active_index());
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), wallet_item.url));

  // Click wallet item and then first wallet tab(tab index 0) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(0, tab_model()->active_index());

  // Click wallet item and then second wallet tab(index 2) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(2, tab_model()->active_index());

  // Click wallet item and then first wallet tab(index 0) is activated.
  SimulateSidebarItemClickAt(wallet_item_index);
  EXPECT_EQ(0, tab_model()->active_index());

  auto* browser2 = CreateBrowser(browser()->profile());
  ui_test_utils::WaitUntilBrowserBecomeActive(browser2);

  // |browser2| doesn't have any wallet tab. So, clicking the wallet sidebar
  // item activates the other browser's first wallet tab and re-activates its
  // window. The activation waiter must be created before the activating call.
  ui_test_utils::BrowserActivationWaiter activation_waiter(browser());
  auto* browser2_controller = browser2->GetFeatures().sidebar_controller();
  auto browser2_wallet_item_index = browser2_controller->model()->GetIndexOf(
      SidebarItem::BuiltInItemType::kWallet);
  ASSERT_TRUE(browser2_wallet_item_index.has_value());
  browser2_controller->ActivateItemAt(browser2_wallet_item_index.value());
  activation_waiter.WaitForActivation();

  EXPECT_TRUE(BrowserWindow::FromBrowser(browser())->IsActive());
  EXPECT_EQ(0, tab_model()->active_index());
}
#endif  // BUILDFLAG(ENABLE_BRAVE_WALLET)

}  // namespace sidebar
