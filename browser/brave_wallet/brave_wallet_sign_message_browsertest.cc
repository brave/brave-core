/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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
    brave_wallet::SetDefaultEthereumWallet(
        browser()->profile()->GetPrefs(),
        brave_wallet::mojom::DefaultWallet::BraveWallet);
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    test_data_dir = test_data_dir.AppendASCII("brave-wallet");
    https_server_.ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server()->Start());

    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
    keyring_service_ = brave_wallet_service_->keyring_service();
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  void RestoreWallet() {
    ASSERT_TRUE(keyring_service_->RestoreWalletSync(
        kMnemonicDripCaution, kTestWalletPassword, false));
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
    ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
              granted);
  }
  void CallEthereumEnable() {
    ASSERT_TRUE(ExecJs(web_contents(), "ethereumEnable()"));
    EXPECT_TRUE(WaitForWalletBubble(web_contents()));
  }
  std::string GetSIWEMessage(const std::string& account,
                             const std::string& uri = "a.com") {
    return base::StringPrintf(
        "%s wants you to sign in with your Ethereum account:\n"
        "%s\n\n\n"
        "URI: %s\n"
        "Version: 1\n"
        "Chain ID: 1\n"
        "Nonce: 32891756\n"
        "Issued At: 2021-09-30T16:25:24Z)",
        https_server()->GetOrigin("a.com").Serialize().c_str(), account.c_str(),
        https_server()->GetURL(uri, "/sign_message.html").spec().c_str());
  }

 protected:
  raw_ptr<BraveWalletService, DanglingUntriaged> brave_wallet_service_ =
      nullptr;
  std::vector<std::string> methods_{"signMessage", "signMessageViaSend",
                                    "signMessageViaSend2",
                                    "signMessageViaSendAsync"};

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServer https_server_;
  raw_ptr<KeyringService, DanglingUntriaged> keyring_service_ = nullptr;
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
        true, request_index++, nullptr, std::nullopt);
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()").ExtractString(),
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
        false, request_index++, nullptr, std::nullopt);
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()").ExtractString(),
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
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()").ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
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
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()").ExtractString(),
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
    EXPECT_EQ(EvalJs(web_contents(), "getSignMessageResult()").ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  }
}

IN_PROC_BROWSER_TEST_F(BraveWalletSignMessageBrowserTest, SIWE) {
  RestoreWallet();
  GURL url = https_server()->GetURL("a.com", "/sign_message.html");

  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);
  size_t request_index = 0;
  struct {
    std::string api_account;
    std::string msg_account;
  } cases[]{
      // checksum, checksum
      {"0x084DCb94038af1715963F149079cE011C4B22961",
       "0x084DCb94038af1715963F149079cE011C4B22961"},
      // all lower case, checksum
      {"0x084dcb94038af1715963f149079ce011c4b22961",
       "0x084DCb94038af1715963F149079cE011C4B22961"},
      // checksum, all lower case
      {"0x084DCb94038af1715963F149079cE011C4B22961",
       "0x084dcb94038af1715963f149079ce011c4b22961"},
      // all lower case, all lower case
      {"0x084dcB94038AF1715963f149079Ce011c4b22961",
       "0x084dcb94038af1715963f149079ce011c4b22961"},
      // all upper case, checksum
      {"0x084DCB94038AF1715963F149079CE011C4B22961",
       "0x084DCb94038af1715963F149079cE011C4B22961"},
      // checksum, all upper case
      {"0x084DCb94038af1715963F149079cE011C4B22961",
       "0x084DCB94038AF1715963F149079CE011C4B22961"},
      // all upper case, all upper case
      {"0x084DCB94038AF1715963F149079CE011C4B22961",
       "0x084DCB94038AF1715963F149079CE011C4B22961"},
      // all lower case, all upper case
      {"0x084dcb94038af1715963f149079ce011c4b22961",
       "0x084DCB94038AF1715963F149079CE011C4B22961"},
      // all upper case, all lower case
      {"0x084DCB94038AF1715963F149079CE011C4B22961",
       "0x084dcb94038af1715963f149079ce011c4b22961"},
  };
  for (const std::string& method : methods_) {
    for (const auto& valid_case : cases) {
      SCOPED_TRACE(testing::Message()
                   << "method:" << method
                   << ", api account:" << valid_case.api_account
                   << ", msg account:" << valid_case.msg_account);
      ASSERT_TRUE(ExecJs(
          web_contents(),
          base::StringPrintf(
              "%s('%s', '%s')", method.c_str(), valid_case.api_account.c_str(),
              ToHex(GetSIWEMessage(valid_case.msg_account)).c_str())));
      // uri has different origin
      ASSERT_TRUE(ExecJs(
          web_contents(),
          base::StringPrintf(
              "%s('%s', '%s')", method.c_str(), valid_case.api_account.c_str(),
              ToHex(GetSIWEMessage(valid_case.msg_account, "www.a.com"))
                  .c_str())));
      // Wait for EthereumProviderImpl::ContinueSignMessage
      base::RunLoop().RunUntilIdle();
      EXPECT_TRUE(WaitForWalletBubble(web_contents()));
      brave_wallet_service_->NotifySignMessageRequestProcessed(
          true, request_index++, nullptr, std::nullopt);
      // port is dynamic
      EXPECT_TRUE(EvalJs(web_contents(), "getSignMessageResult()")
                      .ExtractString()
                      .starts_with("0x"));
    }
  }
}

}  // namespace brave_wallet
