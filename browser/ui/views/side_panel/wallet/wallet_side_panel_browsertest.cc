/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/ui/sidebar/sidebar_utils.h"
#include "brave/browser/ui/webui/brave_wallet/wallet_panel/wallet_panel_ui.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"
#include "chrome/browser/ui/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/side_panel/side_panel_ui.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
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
  ASSERT_TRUE(brave_wallet::IsAllowed(browser()->GetProfile()->GetPrefs()));
  ASSERT_TRUE(sidebar::CanUseSidebar(browser()));

  auto* registry = SidePanelRegistry::From(browser()->GetActiveTabInterface());
  ASSERT_TRUE(registry);
  EXPECT_TRUE(
      registry->GetEntryForKey(SidePanelEntry::Key(SidePanelEntryId::kWallet)));
}

IN_PROC_BROWSER_TEST_F(WalletSidePanelBrowserTest, ShowsWalletSidePanel) {
  auto* panel_ui = SidePanelUI::From(browser());
  ASSERT_TRUE(panel_ui);

  panel_ui->Show(SidePanelEntryId::kWallet);
  EXPECT_TRUE(panel_ui->IsSidePanelEntryShowing(
      SidePanelEntry::Key(SidePanelEntryId::kWallet)));
  EXPECT_EQ(SidePanelEntryId::kWallet, panel_ui->GetCurrentEntryId());
}

// Feature is disabled by default; no need to disable it explicitly.
using WalletSidePanelDisabledBrowserTest = InProcessBrowserTest;

IN_PROC_BROWSER_TEST_F(WalletSidePanelDisabledBrowserTest,
                       DoesNotRegisterContextualEntry) {
  auto* registry = SidePanelRegistry::From(browser()->GetActiveTabInterface());
  ASSERT_TRUE(registry);
  EXPECT_FALSE(
      registry->GetEntryForKey(SidePanelEntry::Key(SidePanelEntryId::kWallet)));
}

// The side panel's handler must serve the tab the panel is embedded in, not
// any other window's active tab.
IN_PROC_BROWSER_TEST_F(WalletSidePanelBrowserTest,
                       PanelHandlerResolvesEmbeddingTab) {
  constexpr char kTestSolAccount[] =
      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8";

  auto* panel_ui = SidePanelUI::From(browser());
  ASSERT_TRUE(panel_ui);
  panel_ui->Show(SidePanelEntryId::kWallet);
  ASSERT_TRUE(base::test::RunUntil([&] {
    return panel_ui->IsSidePanelEntryShowing(
        SidePanelEntry::Key(SidePanelEntryId::kWallet));
  }));

  content::WebContents* panel_contents =
      panel_ui->GetWebContentsForTest(SidePanelEntryId::kWallet);
  ASSERT_TRUE(panel_contents);
  ASSERT_TRUE(base::test::RunUntil([&] {
    auto* webui = panel_contents->GetWebUI();
    return webui && webui->GetController() &&
           webui->GetController()->GetAs<WalletPanelUI>() != nullptr;
  }));
  auto* wallet_panel =
      panel_contents->GetWebUI()->GetController()->GetAs<WalletPanelUI>();
  ASSERT_TRUE(wallet_panel);
  ASSERT_TRUE(base::test::RunUntil(
      [&] { return wallet_panel->panel_handler_for_testing() != nullptr; }));

  // A connection recorded on another window's tab is not the side panel's.
  auto* window2 = CreateBrowser(browser()->GetProfile());
  content::WebContents* contents_w2 =
      window2->tab_strip_model()->GetActiveWebContents();
  auto* tab_helper_w2 = brave_wallet::BraveWalletTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(contents_w2));
  ASSERT_TRUE(tab_helper_w2);
  tab_helper_w2->AddSolanaConnectedAccount(
      contents_w2->GetPrimaryMainFrame()->GetGlobalId(), kTestSolAccount);

  base::test::TestFuture<bool> connected;
  wallet_panel->panel_handler_for_testing()->IsSolanaAccountConnected(
      kTestSolAccount, connected.GetCallback());
  EXPECT_FALSE(connected.Get());

  // A connection on the embedding tab is reported.
  content::WebContents* contents_a =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto* tab_helper_a = brave_wallet::BraveWalletTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(contents_a));
  ASSERT_TRUE(tab_helper_a);
  tab_helper_a->AddSolanaConnectedAccount(
      contents_a->GetPrimaryMainFrame()->GetGlobalId(), kTestSolAccount);
  base::test::TestFuture<bool> connected_after;
  wallet_panel->panel_handler_for_testing()->IsSolanaAccountConnected(
      kTestSolAccount, connected_after.GetCallback());
  EXPECT_TRUE(connected_after.Get());
}
