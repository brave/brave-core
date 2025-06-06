/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_command_controller.h"

#include "base/test/scoped_feature_list.h"
#include "brave/app/brave_command_ids.h"
#include "brave/browser/ui/browser_commands.h"
#include "chrome/browser/chrome_content_browser_client.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/common/content_client.h"

namespace {
// Initializing CertVerifierServiceTimeUpdater from
// SystemNetworkContextManager causes crash during the teardown because
// TestingBrowserProcess::network_time_tracker is destroyed later than
// CertVerifierServiceTimeUpdater. This client should be used by default
// on brave's unittest target.
class ChromeContentBrowserClientWithoutNetworkServiceInitialization
    : public ChromeContentBrowserClient {
 public:
  // content::ContentBrowserClient:
  // Skip some production Network Service code that doesn't work in unit tests.
  void OnNetworkServiceCreated(
      network::mojom::NetworkService* network_service) override {}
};
}  // namespace

class BraveBrowserCommandControllerTest : public BrowserWithTestWindowTest {
 public:
  BraveBrowserCommandControllerTest() = default;

  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();
    SetBrowserClientForTesting(&test_browser_client_);
  }

 private:
  // To run AddTab(), ExtensionWebContentsObserver should be created first.
  // It's created by ChromeContentBrowserClient::OnWebContentsCreated().
  ChromeContentBrowserClientWithoutNetworkServiceInitialization
      test_browser_client_;
};

class BraveBrowserCommandControllerWithSideBySideTest
    : public BraveBrowserCommandControllerTest {
 public:
  BraveBrowserCommandControllerWithSideBySideTest() {
    scoped_features_.InitWithFeatures(
        /*enabled_features*/ {features::kSideBySide}, {});
  }

 private:
  base::test::ScopedFeatureList scoped_features_;
};

TEST_F(BraveBrowserCommandControllerWithSideBySideTest, CommandEnabledTest) {
  GURL about_blank(url::kAboutBlankURL);
  AddTab(browser(), about_blank);
  AddTab(browser(), about_blank);
  EXPECT_EQ(2, browser()->tab_strip_model()->count());

  // When active(selected) tab is not split tab, only |IDC_NEW_SPLIT_VIEW|
  // command is enabled.
  browser()->tab_strip_model()->ActivateTabAt(1);
  EXPECT_EQ(1, browser()->tab_strip_model()->active_index());
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_NEW_SPLIT_VIEW));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_TILE_TABS));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_BREAK_TILE));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_SWAP_SPLIT_VIEW));

  // When active(selected) tab is split tab, only |IDC_BREAK_TILE| and
  // |IDC_SWAP_SPLIT_VIEW| commands are enabled.
  CommandUpdater* updater = browser()->command_controller();
  updater->ExecuteCommand(IDC_NEW_SPLIT_VIEW);
  EXPECT_EQ(3, browser()->tab_strip_model()->count());
  EXPECT_EQ(2, browser()->tab_strip_model()->active_index());
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_NEW_SPLIT_VIEW));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_TILE_TABS));
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_BREAK_TILE));
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_SWAP_SPLIT_VIEW));

  // Only |IDC_NEW_SPLIT_VIEW| is enabled after removing split tabs.
  updater->ExecuteCommand(IDC_BREAK_TILE);
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_NEW_SPLIT_VIEW));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_TILE_TABS));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_BREAK_TILE));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_SWAP_SPLIT_VIEW));

  // Only |IDC_TILE_TABS| is enabled after selecting two tabs.
  browser()->tab_strip_model()->ActivateTabAt(0);
  browser()->tab_strip_model()->SelectTabAt(1);
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_NEW_SPLIT_VIEW));
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_TILE_TABS));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_BREAK_TILE));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_SWAP_SPLIT_VIEW));

  // |IDC_TILE_TABS| is disabled if selected tabs count is not 2.
  browser()->tab_strip_model()->ActivateTabAt(0);
  browser()->tab_strip_model()->SelectTabAt(1);
  browser()->tab_strip_model()->SelectTabAt(2);
  EXPECT_TRUE(chrome::IsCommandEnabled(browser(), IDC_NEW_SPLIT_VIEW));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_TILE_TABS));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_BREAK_TILE));
  EXPECT_FALSE(chrome::IsCommandEnabled(browser(), IDC_SWAP_SPLIT_VIEW));
}
