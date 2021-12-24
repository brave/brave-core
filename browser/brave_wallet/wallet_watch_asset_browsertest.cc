/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace brave_wallet {

class WalletWatchAssetBrowserTest : public InProcessBrowserTest {
 public:
  WalletWatchAssetBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~WalletWatchAssetBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");

    brave::RegisterPathProvider();
    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII("brave-wallet");
    https_server_.ServeFilesFromDirectory(test_data_dir);
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
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

  void RestoreWallet() {
    const char mnemonic[] =
        "drip caution abandon festival order clown oven regular absorb "
        "evidence crew where";
    base::RunLoop run_loop;
    keyring_service_->RestoreWallet(
        mnemonic, "brave123", false,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  std::vector<mojom::BlockchainTokenPtr> GetUserAssets() const {
    base::RunLoop run_loop;
    std::vector<mojom::BlockchainTokenPtr> tokens_out;
    brave_wallet_service_->GetUserAssets(
        GetCurrentChainId(browser()->profile()->GetPrefs()),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> tokens) {
              for (const auto& token : tokens)
                tokens_out.push_back(token.Clone());
              run_loop.Quit();
            }));
    run_loop.Run();
    return tokens_out;
  }

  void WaitForWalletWatchAssetResultReady() {
    content::DOMMessageQueue message_queue;
    std::string message;
    EXPECT_TRUE(message_queue.WaitForMessage(&message));
    EXPECT_EQ("\"result ready\"", message);
  }

 protected:
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  std::vector<std::string> methods_{"request", "send1", "send2", "sendAsync"};
  std::vector<std::string> addresses_{
      "0x6B175474E89094C44Da98b954EedeAC495271d0F",
      "0xdAC17F958D2ee523a2206206994597C13D831ec7",
      "0xc00e94Cb662C3520282E6f5717214004A7f26888",
      "0x4Fabb145d64652a948d72533023f6E7A623C7C53"};
  std::vector<std::string> symbols_{"USDC", "USDT", "GUSD", "BUSD"};
  std::vector<int> decimals_{6, 6, 2, 18};

 private:
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, UserApprovedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  size_t asset_num = GetUserAssets().size();
  for (size_t i = 0; i < methods_.size(); i++) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("wallet_watchAsset('%s', 'ERC20', '%s', '%s', %d)",
                           methods_[i].c_str(), addresses_[i].c_str(),
                           symbols_[i].c_str(), decimals_[i])));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    brave_wallet_service_->NotifyAddSuggestTokenRequestsProcessed(
        true, {addresses_[i]});

    WaitForWalletWatchAssetResultReady();
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(EvalJs(web_contents(), "getWalletWatchAssetResult()",
                       content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                    .ExtractBool());
  }
  EXPECT_EQ(asset_num + methods_.size(), GetUserAssets().size());
}

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, UserRejectedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  size_t asset_num = GetUserAssets().size();
  for (size_t i = 0; i < methods_.size(); i++) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("wallet_watchAsset('%s', 'ERC20', '%s', '%s', %d)",
                           methods_[i].c_str(), addresses_[i].c_str(),
                           symbols_[i].c_str(), decimals_[i])));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    brave_wallet_service_->NotifyAddSuggestTokenRequestsProcessed(
        false, {addresses_[i]});

    WaitForWalletWatchAssetResultReady();
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(EvalJs(web_contents(), "getWalletWatchAssetResult()",
                        content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                     .ExtractBool());
  }
  EXPECT_EQ(asset_num, GetUserAssets().size());
}

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, InvalidTypeParam) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  for (size_t i = 0; i < methods_.size(); i++) {
    EXPECT_EQ(EvalJs(web_contents(),
                     base::StringPrintf(
                         "wallet_watchAsset('%s', 'ERC721', '%s', '%s', %d)",
                         methods_[i].c_str(), addresses_[i].c_str(),
                         symbols_[i].c_str(), decimals_[i]),
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              "Asset of type 'ERC721' not supported");
  }
}

}  // namespace brave_wallet
