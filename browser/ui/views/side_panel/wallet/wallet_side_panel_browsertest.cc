/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

// Verifies Wallet registers as a contextual (per-tab) side panel entry when
// kBraveWalletSidePanel is enabled.
class WalletSidePanelBrowserTest : public InProcessBrowserTest {
 public:
  WalletSidePanelBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletSidePanel);
  }
  WalletSidePanelBrowserTest(const WalletSidePanelBrowserTest&) = delete;
  WalletSidePanelBrowserTest& operator=(const WalletSidePanelBrowserTest&) =
      delete;
  ~WalletSidePanelBrowserTest() override = default;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WalletSidePanelBrowserTest,
                       RegistersContextualEntryForTab) {
  ASSERT_TRUE(brave_wallet::IsAllowed(browser()->profile()->GetPrefs()));
  ASSERT_TRUE(sidebar::CanUseSidebar(browser()));

  auto* registry = SidePanelRegistry::From(browser()->GetActiveTabInterface());
  ASSERT_TRUE(registry);
  EXPECT_TRUE(
      registry->GetEntryForKey(SidePanelEntry::Key(SidePanelEntryId::kWallet)));
}

IN_PROC_BROWSER_TEST_F(WalletSidePanelBrowserTest, ShowsWalletSidePanel) {
  auto* panel_ui = browser()->GetFeatures().side_panel_ui();
  ASSERT_TRUE(panel_ui);

  panel_ui->Show(SidePanelEntryId::kWallet);
  EXPECT_TRUE(panel_ui->IsSidePanelEntryShowing(
      SidePanelEntry::Key(SidePanelEntryId::kWallet)));
  EXPECT_EQ(SidePanelEntryId::kWallet, panel_ui->GetCurrentEntryId());
}

class WalletSidePanelDisabledBrowserTest : public InProcessBrowserTest {
 public:
  WalletSidePanelDisabledBrowserTest() {
    scoped_feature_list_.InitAndDisableFeature(
        brave_wallet::features::kBraveWalletSidePanel);
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WalletSidePanelDisabledBrowserTest,
                       DoesNotRegisterContextualEntry) {
  auto* registry = SidePanelRegistry::From(browser()->GetActiveTabInterface());
  ASSERT_TRUE(registry);
  EXPECT_FALSE(
      registry->GetEntryForKey(SidePanelEntry::Key(SidePanelEntryId::kWallet)));
}
