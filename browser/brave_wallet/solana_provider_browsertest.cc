/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {
constexpr char first_account[] = "8J7fu34oNJSKXcauNQMXRdKAHY7zQ7rEaQng8xtQNpSu";
}

class SolanaProviderTest : public InProcessBrowserTest {
 public:
  SolanaProviderTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    feature_list_.InitWithFeatures(
        {brave_wallet::features::kBraveWalletSolanaFeature,
         brave_wallet::features::kBraveWalletSolanaProviderFeature},
        {});
  }

  ~SolanaProviderTest() override = default;

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII("brave-wallet");
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server()->Start());

    keyring_service_ =
        KeyringServiceFactory::GetServiceForContext(browser()->profile());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  void RestoreWallet() {
    const char mnemonic[] =
        "scare piece awesome elite long drift control cabbage glass dash coral "
        "angry";
    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        mnemonic, "brave123", false,
        base::BindLambdaForTesting([&run_loop](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddAccount() {
    base::RunLoop run_loop;
    keyring_service_->AddAccount(
        "Account 1", mojom::CoinType::SOL,
        base::BindLambdaForTesting([&run_loop](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void SetSelectedAccount(const std::string& address) {
    base::RunLoop run_loop;
    keyring_service_->SetSelectedAccount(
        address, mojom::CoinType::SOL,
        base::BindLambdaForTesting([&run_loop](bool success) {
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void UserGrantPermission(bool granted) {
    if (granted)
      permissions::BraveWalletPermissionContext::AcceptOrCancel(
          std::vector<std::string>{first_account}, web_contents());
    else
      permissions::BraveWalletPermissionContext::Cancel(web_contents());
    ASSERT_EQ(EvalJs(web_contents(), "getConnectedAccount()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              granted ? first_account : "");
  }

  void CallSolanaConnect(bool is_expect_bubble = true) {
    ASSERT_TRUE(ExecJs(web_contents(), "solanaConnect()"));
    base::RunLoop().RunUntilIdle();
    if (is_expect_bubble) {
      ASSERT_TRUE(
          brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
              ->IsShowingBubble());
    }
  }

  void CallSolanaDisconnect() {
    ASSERT_TRUE(EvalJs(web_contents(), "solanaDisconnect()",
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                    .ExtractBool());
  }

  bool IsSolanaConnected() {
    return EvalJs(web_contents(), "isSolanaConnected()",
                  content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
        .ExtractBool();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusAndPermission) {
  RestoreWallet();
  AddAccount();
  SetSelectedAccount(first_account);
  GURL url = https_server()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_FALSE(IsSolanaConnected());
  CallSolanaConnect();
  UserGrantPermission(true);
  EXPECT_TRUE(IsSolanaConnected());

  // Removing solana permission doesn't affect connected status.
  host_content_settings_map()->ClearSettingsForOneType(
      ContentSettingsType::BRAVE_SOLANA);
  EXPECT_TRUE(IsSolanaConnected());

  // Doing connect again and reject it doesn't affect connected status either.
  CallSolanaConnect();
  UserGrantPermission(false);
  EXPECT_TRUE(IsSolanaConnected());

  // Only disconnect will set connected status to false.
  CallSolanaDisconnect();
  EXPECT_FALSE(IsSolanaConnected());
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, ConnectedStatusInMultiFrames) {
  RestoreWallet();
  AddAccount();
  SetSelectedAccount(first_account);
  GURL url = https_server()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  ASSERT_FALSE(IsSolanaConnected());
  CallSolanaConnect();
  UserGrantPermission(true);
  // First tab is now connected.
  EXPECT_TRUE(IsSolanaConnected());
  // Add same url at second tab
  ASSERT_TRUE(AddTabAtIndex(1, url, ui::PAGE_TRANSITION_TYPED));
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 1);
  // Connected status of second tab is separate from first tab.
  EXPECT_FALSE(IsSolanaConnected());
  // Doing successful connect and disconnect shouldn't affect first tab.
  // Since a.test already has the permission so connect would success without
  // asking.
  CallSolanaConnect(false);
  EXPECT_TRUE(IsSolanaConnected());
  CallSolanaDisconnect();
  EXPECT_FALSE(IsSolanaConnected());

  // Swtich back to first tab and it should still be connected,
  browser()->tab_strip_model()->ActivateTabAt(0);
  ASSERT_EQ(browser()->tab_strip_model()->active_index(), 0);
  EXPECT_TRUE(IsSolanaConnected());
}

}  // namespace brave_wallet
