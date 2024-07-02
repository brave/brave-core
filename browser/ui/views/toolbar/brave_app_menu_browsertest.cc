/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/skus/common/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/app_menu.h"
#include "chrome/browser/ui/views/toolbar/browser_app_menu_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/test_utils.h"
#include "ui/base/ui_base_features.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/view_utils.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/browser/brave_vpn/brave_vpn_service_factory.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/common/features.h"
#endif

class BraveAppMenuBrowserTest : public InProcessBrowserTest {
 public:
  BraveAppMenuBrowserTest() {
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, brave_vpn::features::kBraveVPN}, {});
#endif
  }

  ~BraveAppMenuBrowserTest() override = default;

  BrowserAppMenuButton* menu_button() {
    return BrowserView::GetBrowserViewForBrowser(browser())
        ->toolbar()
        ->app_menu_button();
  }

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  void SetPurchasedUserForBraveVPN(Browser* browser, bool purchased) {
    auto* service =
        brave_vpn::BraveVpnServiceFactory::GetForProfile(browser->profile());
    ASSERT_TRUE(!!service);
    auto target_state = purchased
                            ? brave_vpn::mojom::PurchasedState::PURCHASED
                            : brave_vpn::mojom::PurchasedState::NOT_PURCHASED;
    service->SetPurchasedState(skus::GetDefaultEnvironment(), target_state);
    // Call explicitely to update vpn commands status because mojo works in
    // async way.
    static_cast<chrome::BraveBrowserCommandController*>(
        browser->command_controller())
        ->OnPurchasedStateChanged(target_state, std::nullopt);
  }

  base::test::ScopedFeatureList scoped_feature_list_;
#endif
};

#if BUILDFLAG(ENABLE_BRAVE_VPN)
// Check toggle menu item has additional toggle button for purchased user.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, PurchasedVPN) {
  SetPurchasedUserForBraveVPN(browser(), true);
  menu_button()->ShowMenu(views::MenuRunner::NO_FLAGS);
  views::MenuItemView* menu_root = menu_button()->app_menu()->root_menu_item();
  auto* toggle_menu_item = menu_root->GetMenuItemByID(IDC_TOGGLE_BRAVE_VPN);
  ASSERT_TRUE(!!toggle_menu_item);
  const int last_item_index = toggle_menu_item->children().size() - 1;
  auto* toggle_button = views::AsViewClass<views::ToggleButton>(
      toggle_menu_item->children()[last_item_index]);
  ASSERT_NE(nullptr, toggle_button);
}

// Check app menu has show vpn panel menu item for not purchased user.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTest, NotPurchasedVPN) {
  SetPurchasedUserForBraveVPN(browser(), false);
  menu_button()->ShowMenu(views::MenuRunner::NO_FLAGS);
  views::MenuItemView* menu_root = menu_button()->app_menu()->root_menu_item();
  EXPECT_TRUE(!!menu_root->GetMenuItemByID(IDC_SHOW_BRAVE_VPN_PANEL));
}
#endif

class BraveAppMenuBrowserTestWithChromeRefresh2023
    : public BraveAppMenuBrowserTest {
 public:
  BraveAppMenuBrowserTestWithChromeRefresh2023() = default;
  ~BraveAppMenuBrowserTestWithChromeRefresh2023() override = default;

  void SetUp() override {
    BraveAppMenuBrowserTest::SetUp();
  }

  base::test::ScopedFeatureList feature_list_;
};

// Test originally introduced before the chrome-refresh-2023 cleanup, to check
// if sidebar crashes when opening.
IN_PROC_BROWSER_TEST_F(BraveAppMenuBrowserTestWithChromeRefresh2023,
                       AppMenuOpeningTest) {
  // Open app menu to check it doesn't make crash.
  menu_button()->ShowMenu(views::MenuRunner::NO_FLAGS);
}
