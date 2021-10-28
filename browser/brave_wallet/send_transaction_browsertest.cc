/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/common/brave_paths.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse());
  http_response->set_code(net::HTTP_OK);
  http_response->set_content_type("text/html");
  std::string request_path = request.GetURL().path();
  http_response->set_content(R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": "0x00000000000009604"
  })");
  return std::move(http_response);
}

}  // namespace

namespace brave_wallet {

class TestEthTxControllerObserver
    : public brave_wallet::mojom::EthTxControllerObserver {
 public:
  TestEthTxControllerObserver() {}
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx) override {}

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx) override {
    if (tx->tx_status == mojom::TransactionStatus::Submitted) {
      run_loop_approved_->Quit();
    } else if (tx->tx_status == mojom::TransactionStatus::Rejected) {
      run_loop_rejected_->Quit();
    }
  }

  void WaitForApprovedStatus() {
    run_loop_approved_ = std::make_unique<base::RunLoop>();
    run_loop_approved_->Run();
  }

  void WaitForRjectedStatus() {
    run_loop_rejected_ = std::make_unique<base::RunLoop>();
    run_loop_rejected_->Run();
  }

  mojo::PendingRemote<brave_wallet::mojom::EthTxControllerObserver>
  GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

 private:
  mojo::Receiver<brave_wallet::mojom::EthTxControllerObserver>
      observer_receiver_{this};
  std::unique_ptr<base::RunLoop> run_loop_approved_;
  std::unique_ptr<base::RunLoop> run_loop_rejected_;
};

class SendTransactionBrowserTest : public InProcessBrowserTest {
 public:
  SendTransactionBrowserTest()
      : https_server_for_files_(net::EmbeddedTestServer::TYPE_HTTPS),
        https_server_for_rpc_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~SendTransactionBrowserTest() override = default;

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
    https_server_for_files()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_for_files()->Start());

    brave_wallet_service_ =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
    keyring_controller_ =
        KeyringControllerFactory::GetControllerForContext(browser()->profile());
    eth_tx_controller_ =
        EthTxControllerFactory::GetControllerForContext(browser()->profile());
    eth_json_rpc_controller_ =
        RpcControllerFactory::GetControllerForContext(browser()->profile());

    eth_tx_controller_->AddObserver(observer()->GetReceiver());

    StartRPCServer(base::BindRepeating(&HandleRequest));
  }

  void StartRPCServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_for_rpc()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_for_rpc()->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_for_rpc()->Start());
    eth_json_rpc_controller_->SetCustomNetworkForTesting(
        "0x1", https_server_for_rpc()->base_url());
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  HostContentSettingsMap* host_content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(browser()->profile());
  }

  net::EmbeddedTestServer* https_server_for_files() {
    return &https_server_for_files_;
  }
  net::EmbeddedTestServer* https_server_for_rpc() {
    return &https_server_for_rpc_;
  }
  TestEthTxControllerObserver* observer() { return &observer_; }

  void RestoreWallet() {
    const char mnemonic[] =
        "drip caution abandon festival order clown oven regular absorb "
        "evidence crew where";
    base::RunLoop run_loop;
    keyring_controller_->RestoreWallet(
        mnemonic, "brave123", false,
        base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void LockWallet() {
    keyring_controller_->Lock();
    // Needed so KeyringControllerObserver::Locked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  void UnlockWallet() {
    base::RunLoop run_loop;
    keyring_controller_->Unlock("brave123",
                                base::BindLambdaForTesting([&](bool success) {
                                  ASSERT_TRUE(success);
                                  run_loop.Quit();
                                }));
    run_loop.Run();
    // Needed so KeyringControllerObserver::Unlocked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  void AddAccount(const std::string& account_name) {
    base::RunLoop run_loop;
    keyring_controller_->AddAccount(
        account_name, base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void SetSelectedAccount(const std::string& address) {
    base::RunLoop run_loop;
    keyring_controller_->SetSelectedAccount(
        address, base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void UserGrantPermission(bool granted) {
    std::string expected_address = "undefined";
    if (granted) {
      permissions::BraveEthereumPermissionContext::AcceptOrCancel(
          std::vector<std::string>{from()}, web_contents());
      expected_address = from();
    } else {
      permissions::BraveEthereumPermissionContext::Cancel(web_contents());
    }
    ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractBool(),
              granted);
    // Check that window.ethereum.selectedAddress is set correctly
    EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              expected_address);
  }

  void AddEthereumPermission(const GURL& url, size_t from_index = 0) {
    AddEthereumPermission(url, from(from_index));
  }
  void AddEthereumPermission(const GURL& url, const std::string& address) {
    GURL sub_request_origin;
    ASSERT_TRUE(
        brave_wallet::GetSubRequestOrigin(url, address, &sub_request_origin));
    host_content_settings_map()->SetContentSettingDefaultScope(
        sub_request_origin, url, ContentSettingsType::BRAVE_ETHEREUM,
        ContentSetting::CONTENT_SETTING_ALLOW);
    // Needed so content settings observer handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  std::string from(size_t index = 0) {
    if (index == 0) {
      return "0x084DCb94038af1715963F149079cE011C4B22961";
    }
    if (index == 1) {
      return "0xE60A2209372AF1049C4848B1bF0136258c35f268";
    }
    return "";
  }

  void ApproveTransaction(const std::string& tx_meta_id) {
    base::RunLoop run_loop;
    eth_tx_controller_->ApproveTransaction(
        tx_meta_id, base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          observer()->WaitForApprovedStatus();
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RejectTransaction(const std::string& tx_meta_id) {
    base::RunLoop run_loop;
    eth_tx_controller_->RejectTransaction(
        tx_meta_id, base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          observer()->WaitForRjectedStatus();
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestUserApproved(const std::string& test_method) {
    RestoreWallet();
    GURL url =
        https_server_for_files()->GetURL("a.com", "/send_transaction.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    WaitForLoadStop(web_contents());

    EXPECT_EQ(EvalJs(web_contents(), "getChainId()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              "0x1");

    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    UserGrantPermission(true);
    ASSERT_TRUE(
        ExecJs(web_contents(),
               base::StringPrintf(
                   "sendTransaction(false, '%s', "
                   "'0x084DCb94038af1715963F149079cE011C4B22961', "
                   "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');",
                   test_method.c_str())));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());

    auto infos = GetAllTransactionInfo();
    EXPECT_EQ(1UL, infos.size());
    EXPECT_TRUE(
        base::EqualsCaseInsensitiveASCII(from(), infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[0]->tx_status);
    EXPECT_TRUE(infos[0]->tx_data->base_data->nonce.empty());

    ApproveTransaction(infos[0]->id);

    infos = GetAllTransactionInfo();
    EXPECT_EQ(1UL, infos.size());
    EXPECT_TRUE(
        base::EqualsCaseInsensitiveASCII(from(), infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Submitted, infos[0]->tx_status);
    EXPECT_FALSE(infos[0]->tx_hash.empty());
    EXPECT_EQ(infos[0]->tx_data->base_data->nonce, "0x9604");

    EXPECT_EQ(EvalJs(web_contents(), "getSendTransactionResult()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              "0x00000000000009604");
  }

  void TestUserRejected(const std::string& test_method) {
    RestoreWallet();
    GURL url =
        https_server_for_files()->GetURL("a.com", "/send_transaction.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    WaitForLoadStop(web_contents());

    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    UserGrantPermission(true);
    ASSERT_TRUE(
        ExecJs(web_contents(),
               base::StringPrintf(
                   "sendTransaction(false, '%s', "
                   "'0x084DCb94038af1715963F149079cE011C4B22961', "
                   "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');",
                   test_method.c_str())));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());

    auto infos = GetAllTransactionInfo();
    EXPECT_EQ(1UL, infos.size());
    EXPECT_TRUE(
        base::EqualsCaseInsensitiveASCII(from(), infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[0]->tx_status);
    EXPECT_TRUE(infos[0]->tx_data->base_data->nonce.empty());

    RejectTransaction(infos[0]->id);

    infos = GetAllTransactionInfo();
    EXPECT_EQ(1UL, infos.size());
    EXPECT_TRUE(
        base::EqualsCaseInsensitiveASCII(from(), infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Rejected, infos[0]->tx_status);
    EXPECT_TRUE(infos[0]->tx_hash.empty());
    EXPECT_TRUE(infos[0]->tx_data->base_data->nonce.empty());

    EXPECT_EQ(EvalJs(web_contents(), "getSendTransactionError()",
                     content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                  .ExtractString(),
              l10n_util::GetStringUTF8(
                  IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
  }

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo() {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    eth_tx_controller_->GetAllTransactionInfo(
        from(), base::BindLambdaForTesting(
                    [&](std::vector<mojom::TransactionInfoPtr> v) {
                      transaction_infos = std::move(v);
                      run_loop.Quit();
                    }));
    run_loop.Run();
    return transaction_infos;
  }

 protected:
  BraveWalletService* brave_wallet_service_;

 private:
  TestEthTxControllerObserver observer_;
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServer https_server_for_files_;
  net::test_server::EmbeddedTestServer https_server_for_rpc_;
  KeyringController* keyring_controller_;
  EthTxController* eth_tx_controller_;
  EthJsonRpcController* eth_json_rpc_controller_;
};

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserApprovedRequest) {
  TestUserApproved("request");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserApprovedSend1) {
  TestUserApproved("send1");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserApprovedSend2) {
  TestUserApproved("send2");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserApprovedSendAsync) {
  TestUserApproved("sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserRejectedRequest) {
  TestUserRejected("request");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserRejectedSend1) {
  TestUserRejected("send1");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserRejectedSend2) {
  TestUserRejected("send2");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, UserRejectedSendAsync) {
  TestUserRejected("sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, InvalidAddress) {
  RestoreWallet();
  GURL url =
      https_server_for_files()->GetURL("a.com", "/send_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  EXPECT_TRUE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  UserGrantPermission(true);
  ASSERT_TRUE(ExecJs(web_contents(),
                     "sendTransaction(false, 'request', "
                     "'0x6b1Bd828cF8CE051B6282dCFEf6863746E2E1909', "
                     "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');"));
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  EXPECT_EQ(EvalJs(web_contents(), "getSendTransactionError()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            l10n_util::GetStringUTF8(
                IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, NoEthPermission) {
  RestoreWallet();
  GURL url =
      https_server_for_files()->GetURL("a.com", "/send_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  EXPECT_TRUE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  UserGrantPermission(false);
  ASSERT_TRUE(ExecJs(web_contents(),
                     "sendTransaction(false, 'request', "
                     "'0x084DCb94038af1715963F149079cE011C4B22961', "
                     "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');"));
  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  EXPECT_EQ(EvalJs(web_contents(), "getSendTransactionError()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            l10n_util::GetStringUTF8(
                IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
}

IN_PROC_BROWSER_TEST_F(SendTransactionBrowserTest, SelectedAddress) {
  RestoreWallet();
  AddAccount("account 2");
  GURL url =
      https_server_for_files()->GetURL("a.com", "/send_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  WaitForLoadStop(web_contents());

  EXPECT_EQ(EvalJs(web_contents(), "getChainId()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            "0x1");

  EXPECT_TRUE(
      brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
          ->IsShowingBubble());
  UserGrantPermission(true);

  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            from());

  // Locking the wallet makes the selectedAddress property undefined
  LockWallet();
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            "undefined");

  // Unlock wallet restores the selectedAddress property
  UnlockWallet();
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            from());

  // Changing the selected account doesn't change selectedAddress property
  // because it's not allowed yet.
  SetSelectedAccount(from(1));
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            from());

  // But it does update the selectedAddress if the account is allowed
  AddEthereumPermission(url, 1);
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()",
                   content::EXECUTE_SCRIPT_USE_MANUAL_REPLY)
                .ExtractString(),
            from(1));
}

}  // namespace brave_wallet
