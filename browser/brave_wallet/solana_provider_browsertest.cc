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
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
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
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
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

    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
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

  void LockWallet() {
    keyring_service_->Lock();
    // Needed so KeyringServiceObserver::Locked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
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

  void CallSolanaSignMessage(const std::string& message,
                             const std::string& encoding) {
    ASSERT_TRUE(ExecJs(web_contents(),
                       base::StringPrintf(R"(solanaSignMessage('%s', '%s'))",
                                          message.c_str(), encoding.c_str())));
  }

  std::string GetSignMessageResult() {
    return EvalJs(web_contents(), "getSignMessageResult()",
                  content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
        .ExtractString();
  }

  bool IsSolanaConnected() {
    return EvalJs(web_contents(), "isSolanaConnected()",
                  content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
        .ExtractBool();
  }

  void WaitForResultReady() {
    content::DOMMessageQueue message_queue;
    std::string message;
    EXPECT_TRUE(message_queue.WaitForMessage(&message));
    EXPECT_EQ("\"result ready\"", message);
  }

 protected:
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;

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

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, SignMessage) {
  RestoreWallet();
  AddAccount();
  SetSelectedAccount(first_account);
  GURL url = https_server()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  CallSolanaConnect();
  UserGrantPermission(true);
  ASSERT_TRUE(IsSolanaConnected());

  constexpr char message[] = "bravey baby!";
  size_t request_index = 0;
  CallSolanaSignMessage(message, "utf8");
  EXPECT_TRUE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  // user rejected request
  brave_wallet_service_->NotifySignMessageRequestProcessed(false,
                                                           request_index++);
  WaitForResultReady();
  EXPECT_EQ(GetSignMessageResult(),
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  constexpr char expected_signature[] =
      "98,100,65,130,165,105,247,254,176,58,137,184,149,50,202,4,239,34,179,15,"
      "99,184,125,255,9,227,4,118,70,108,153,191,78,251,150,104,239,24,191,139,"
      "242,54,150,144,96,249,42,106,199,171,222,72,108,190,206,193,130,47,125,"
      "239,173,127,238,11";

  for (const std::string& encoding : {"utf8", "hex", "invalid", ""}) {
    CallSolanaSignMessage(message, encoding);
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    // user approved request
    brave_wallet_service_->NotifySignMessageRequestProcessed(true,
                                                             request_index++);
    WaitForResultReady();
    EXPECT_EQ(GetSignMessageResult(), expected_signature);
  }
}

IN_PROC_BROWSER_TEST_F(SolanaProviderTest, GetPublicKey) {
  RestoreWallet();
  AddAccount();
  SetSelectedAccount(first_account);
  GURL url = https_server()->GetURL("a.test", "/solana_provider.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  constexpr char get_public_key_script[] =
      "window.domAutomationController.send(window.solana."
      "publicKey ? window.solana.publicKey.toString() : '')";

  // Will get null in disconnected state
  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script,
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            "");

  CallSolanaConnect();
  UserGrantPermission(true);
  ASSERT_TRUE(IsSolanaConnected());

  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script,
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            first_account);

  LockWallet();
  // Publickey is still accessible when wallet is locked
  EXPECT_EQ(EvalJs(web_contents(), get_public_key_script,
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            first_account);
}

}  // namespace brave_wallet
