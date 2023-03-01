/* Copyright (c) 2021 The Brave Authors. All rights reserved.
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
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

bool WaitForWalletBubble(content::WebContents* web_contents) {
  auto* tab_helper =
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents);
  if (!tab_helper->IsShowingBubble()) {
    base::RunLoop run_loop;
    tab_helper->SetShowBubbleCallbackForTesting(run_loop.QuitClosure());
    run_loop.Run();
  }

  return tab_helper->IsShowingBubble();
}

}  // namespace

class BraveWalletSignMessageBrowserTest : public InProcessBrowserTest {
 public:
  BraveWalletSignMessageBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~BraveWalletSignMessageBrowserTest() override = default;

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
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

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
  void UserGrantPermission(bool granted) {
    if (granted) {
      permissions::BraveWalletPermissionContext::AcceptOrCancel(
          std::vector<std::string>{
              "0x084DCb94038af1715963F149079cE011C4B22961"},
          mojom::PermissionLifetimeOption::kForever, web_contents());
    } else {
      permissions::BraveWalletPermissionContext::Cancel(web_contents());
    }
    ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractBool(),
              granted);
  }
  void CallEthereumEnable() {
    ASSERT_TRUE(ExecJs(web_contents(), "ethereumEnable()"));
    EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  }

 protected:
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  std::vector<std::string> methods_{"signMessage", "signMessageViaSend",
                                    "signMessageViaSend2",
                                    "signMessageViaSendAsync"};

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, UserApprovedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);
  size_t request_index = 0;
  for (const std::string& method : methods_) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("%s('0x084DCb94038af1715963F149079cE011C4B22961',"
                           " '0xdeadbeef')",
                           method.c_str())));
    // Wait for EthereumProviderImpl::ContinueSignMessage
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(WaitForWalletBubble(web_contents()));
    brave_wallet_service_->NotifySignMessageRequestProcessed(
        true, request_index++, nullptr, absl::nullopt);
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              "0x670651c072cac2a3f93cb862a17378f6849c66b4516e5d5a30210868a2840e"
              "2a6a345a"
              "4f84615c591c1a47260e798babe8f2f0cce03a09dac09df79c55d8e4401b");
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, UserRejectedRequest) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);

  size_t request_index = 0;
  for (const std::string& method : methods_) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("%s('0x084DCb94038af1715963F149079cE011C4B22961',"
                           " '0xdeadbeef')",
                           method.c_str())));
    // Wait for EthereumProviderImpl::ContinueSignMessage
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(WaitForWalletBubble(web_contents()));
    brave_wallet_service_->NotifySignMessageRequestProcessed(
        false, request_index++, nullptr, absl::nullopt);
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, UnknownAddress) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);
  for (const std::string& method : methods_) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("%s('0x6b1Bd828cF8CE051B6282dCFEf6863746E2E1909',"
                           " '0xdeadbeef')",
                           method.c_str())));
    // Wait for EthereumProviderImpl::ContinueSignMessage
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              l10n_util::GetStringFUTF8(
                  IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                  base::ASCIIToUTF16(std::string(
                      "0x6b1Bd828cF8CE051B6282dCFEf6863746E2E1909"))));
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, InvalidAddressParam) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);
  for (const std::string& method : methods_) {
    ASSERT_TRUE(ExecJs(web_contents(), base::StringPrintf("%s(null,"
                                                          " '0xdeadbeef')",
                                                          method.c_str())));
    // Wait for EthereumProviderImpl::ContinueSignMessage
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, NoEthPermission) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(false);
  for (const std::string& method : methods_) {
    ASSERT_TRUE(ExecJs(
        web_contents(),
        base::StringPrintf("%s('0x084DCb94038af1715963F149079cE011C4B22961',"
                           " '0xdeadbeef')",
                           method.c_str())));
    // Wait for EthereumProviderImpl::ContinueSignMessage
    base::RunLoop().RunUntilIdle();
    EXPECT_FALSE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              l10n_util::GetStringFUTF8(
                  IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                  base::ASCIIToUTF16(std::string(
                      "0x084DCb94038af1715963F149079cE011C4B22961"))));
  }
}

}  // namespace brave_wallet
