/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <vector>

#include "base/command_line.h"
#include "base/test/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace brave_wallet {

class WalletWatchAssetBrowserTest : public InProcessBrowserTest {
 public:
  WalletWatchAssetBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}

  ~WalletWatchAssetBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
  }

  void SetUpOnMainThread() override {
    brave_wallet::SetDefaultEthereumWallet(
        browser()->GetProfile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    https_server_.ServeFilesFromDirectory(BraveWalletTestDataFolder());
    https_server_.SetSSLConfig(net::EmbeddedTestServer::CERT_TEST_NAMES);
    ASSERT_TRUE(https_server()->Start());
    // Unknown contracts wait on eth_call before the panel opens. Register
    // these addresses so the test stays on the synchronous path.
    EnsureSuggestedTokensAreKnown();
  }

  void TearDownOnMainThread() override {
    if (restore_mainnet_token_list_) {
      BlockchainRegistry::GetInstance()->UpdateTokenList(
          GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId),
          std::move(original_mainnet_tokens_));
    }
    InProcessBrowserTest::TearDownOnMainThread();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  BraveWalletService* brave_wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
  }

  void EnsureSuggestedTokensAreKnown() {
    auto* registry = BlockchainRegistry::GetInstance();
    bool called = false;
    registry->GetAllTokens(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> tokens) {
              original_mainnet_tokens_ = std::move(tokens);
              called = true;
            }));
    ASSERT_TRUE(called);

    std::vector<mojom::BlockchainTokenPtr> updated;
    updated.reserve(original_mainnet_tokens_.size() + addresses_.size());
    for (const auto& token : original_mainnet_tokens_) {
      updated.push_back(token.Clone());
    }
    bool added = false;
    for (size_t i = 0; i < addresses_.size(); ++i) {
      if (registry->GetTokenByAddress(mojom::kMainnetChainId,
                                      mojom::CoinType::ETH, addresses_[i])) {
        continue;
      }
      updated.push_back(mojom::BlockchainToken::New(
          addresses_[i], symbols_[i], "", false, true, false, false,
          mojom::SPLTokenProgram::kUnsupported, false, false, symbols_[i],
          decimals_[i], true, "", "", mojom::kMainnetChainId,
          mojom::CoinType::ETH, mojom::ZCashTokenType::kNone));
      added = true;
    }
    if (!added) {
      return;
    }
    restore_mainnet_token_list_ = true;
    registry->UpdateTokenList(
        GetTokenListKey(mojom::CoinType::ETH, mojom::kMainnetChainId),
        std::move(updated));
  }

  void RestoreWallet() {
    ASSERT_TRUE(brave_wallet_service()->keyring_service()->RestoreWalletSync(
        kMnemonicDripCaution, kTestWalletPassword, false));
  }

  std::vector<mojom::BlockchainTokenPtr> GetUserAssets() {
    base::RunLoop run_loop;
    std::vector<mojom::BlockchainTokenPtr> tokens_out;
    brave_wallet_service()->GetUserAssets(
        brave_wallet_service()->network_manager()->GetCurrentChainId(
            mojom::CoinType::ETH, std::nullopt),
        mojom::CoinType::ETH,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> tokens) {
              for (const auto& token : tokens) {
                tokens_out.push_back(token.Clone());
              }
              run_loop.Quit();
            }));
    run_loop.Run();
    return tokens_out;
  }

 protected:
  std::vector<std::string> methods_{"request", "send1", "send2", "sendAsync"};
  std::vector<std::string> addresses_{
      "0x6B175474E89094C44Da98b954EedeAC495271d0F",
      "0xdAC17F958D2ee523a2206206994597C13D831ec7",
      "0xc00e94Cb662C3520282E6f5717214004A7f26888",
      "0x4Fabb145d64652a948d72533023f6E7A623C7C53"};
  std::vector<std::string> symbols_{"USDC", "USDT", "GUSD", "BUSD"};
  std::vector<int> decimals_{6, 6, 2, 18};

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::test_server::EmbeddedTestServer https_server_;
  bool restore_mainnet_token_list_ = false;
  std::vector<mojom::BlockchainTokenPtr> original_mainnet_tokens_;
};

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, UserApprovedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  size_t asset_num = GetUserAssets().size();
  for (size_t i = 0; i < methods_.size(); i++) {
    ASSERT_TRUE(
        ExecJs(web_contents(),
               content::JsReplace("wallet_watchAsset($1, 'ERC20', $2, $3, $4)",
                                  methods_[i], addresses_[i], symbols_[i],
                                  decimals_[i]),
               content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    brave_wallet_service()->NotifyAddSuggestTokenRequestsProcessed(
        true, {addresses_[i]});
  }
  EXPECT_EQ(asset_num + methods_.size(), GetUserAssets().size());
}

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, UserRejectedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  size_t asset_num = GetUserAssets().size();
  for (size_t i = 0; i < methods_.size(); i++) {
    ASSERT_TRUE(
        ExecJs(web_contents(),
               content::JsReplace("wallet_watchAsset($1, 'ERC20', $2, $3, $4)",
                                  methods_[i], addresses_[i], symbols_[i],
                                  decimals_[i]),
               content::EXECUTE_SCRIPT_NO_RESOLVE_PROMISES));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    brave_wallet_service()->NotifyAddSuggestTokenRequestsProcessed(
        false, {addresses_[i]});
  }
  EXPECT_EQ(asset_num, GetUserAssets().size());
}

IN_PROC_BROWSER_TEST_F(WalletWatchAssetBrowserTest, InvalidTypeParam) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/wallet_watch_asset.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  for (size_t i = 0; i < methods_.size(); i++) {
    EXPECT_EQ(EvalJs(web_contents(),
                     content::JsReplace(
                         "wallet_watchAsset($1, 'ERC721', $2, $3, $4)",
                         methods_[i], addresses_[i], symbols_[i], decimals_[i]))
                  .ExtractString(),
              "Asset of type 'ERC721' not supported");
  }
}

}  // namespace brave_wallet
