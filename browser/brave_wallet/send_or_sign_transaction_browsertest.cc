/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/memory/raw_ptr.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "content/public/test/test_utils.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

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

constexpr char kSignedTransaction[] =
    "0xf8688296048525f38e9e0082960494084dcb94038af1715963f149079ce011c4b2296211"
    "80820a95a0c58904f26f5ac0e86a292d9a832bbb56ab8d7bfb9f74a5eafa99778bf059ea93"
    "a07db1772583c02ae58637916c03a3a1d9fd98044dd83c52da6870fee25a8575e1";

}  // namespace

namespace brave_wallet {

class TestTxServiceObserver : public brave_wallet::mojom::TxServiceObserver {
 public:
  TestTxServiceObserver() = default;

  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {
    ASSERT_TRUE(tx->tx_data_union->is_eth_tx_data_1559());
    EXPECT_EQ(tx->tx_data_union->get_eth_tx_data_1559()->chain_id.empty(),
              !expect_eip1559_tx_);
    run_loop_new_unapproved_->Quit();
  }

  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx) override {}

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx) override {
    if (tx->tx_status == mojom::TransactionStatus::Rejected) {
      run_loop_rejected_->Quit();
    }
  }

  void OnTxServiceReset() override {}

  void WaitForNewUnapprovedTx() {
    run_loop_new_unapproved_ = std::make_unique<base::RunLoop>();
    run_loop_new_unapproved_->Run();
  }

  void WaitForRejectedStatus() {
    run_loop_rejected_ = std::make_unique<base::RunLoop>();
    run_loop_rejected_->Run();
  }

  mojo::PendingRemote<brave_wallet::mojom::TxServiceObserver> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  void SetExpectEip1559Tx(bool eip1559) { expect_eip1559_tx_ = eip1559; }
  bool expect_eip1559_tx() { return expect_eip1559_tx_; }

 private:
  mojo::Receiver<brave_wallet::mojom::TxServiceObserver> observer_receiver_{
      this};
  std::unique_ptr<base::RunLoop> run_loop_new_unapproved_;
  std::unique_ptr<base::RunLoop> run_loop_rejected_;
  bool expect_eip1559_tx_ = false;
};

class TestJsonRpcServiceObserver : public mojom::JsonRpcServiceObserver {
 public:
  explicit TestJsonRpcServiceObserver(base::OnceClosure callback)
      : callback_(std::move(callback)) {}

  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override {
    std::move(callback_).Run();
  }

  void ChainChangedEvent(const std::string& chain_id,
                         brave_wallet::mojom::CoinType coin,
                         const std::optional<::url::Origin>& origin) override {}

  ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }

  base::OnceClosure callback_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> observer_receiver_{this};
};

class SendOrSignTransactionBrowserTest : public InProcessBrowserTest {
 public:
  SendOrSignTransactionBrowserTest()
      : https_server_for_files_(net::EmbeddedTestServer::TYPE_HTTPS),
        https_server_for_rpc_(net::EmbeddedTestServer::TYPE_HTTPS) {
    scoped_feature_list_.InitAndEnableFeature(
        brave_wallet::features::kNativeBraveWalletFeature);
  }

  ~SendOrSignTransactionBrowserTest() override = default;

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
    https_server_for_files()->ServeFilesFromDirectory(test_data_dir);
    ASSERT_TRUE(https_server_for_files()->Start());

    auto* wallet_service =
        BraveWalletServiceFactory::GetServiceForContext(browser()->profile());
    network_manager_ = wallet_service->network_manager();
    json_rpc_service_ = wallet_service->json_rpc_service();
    json_rpc_service_->SetSkipEthChainIdValidationForTesting(true);
    keyring_service_ = wallet_service->keyring_service();
    tx_service_ = wallet_service->tx_service();

    tx_service_->AddObserver(observer()->GetReceiver());

    StartRPCServer(base::BindRepeating(&HandleRequest));
  }

  void StartRPCServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_for_rpc()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_for_rpc()->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_for_rpc()->Start());
    SetNetworkForTesting(mojom::kLocalhostChainId, std::nullopt);
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
  TestTxServiceObserver* observer() { return &observer_; }

  void RestoreWallet() {
    ASSERT_TRUE(keyring_service_->RestoreWalletSync(
        kMnemonicDripCaution, kTestWalletPassword, false));

    default_account_ =
        keyring_service_->GetAllAccountsSync()->accounts[0]->Clone();
    EXPECT_EQ(base::ToLowerASCII(default_account_->address),
              "0x084dcb94038af1715963f149079ce011c4b22961");
  }

  void LockWallet() {
    keyring_service_->Lock();
    // Needed so KeyringServiceObserver::Locked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  void UnlockWallet() {
    base::RunLoop run_loop;
    keyring_service_->Unlock(kTestWalletPassword,
                             base::BindLambdaForTesting([&](bool success) {
                               ASSERT_TRUE(success);
                               run_loop.Quit();
                             }));
    run_loop.Run();
    // Needed so KeyringServiceObserver::Unlocked handler can be hit
    // which the provider object listens to for the accountsChanged event.
    base::RunLoop().RunUntilIdle();
  }

  mojom::AccountInfoPtr AddAccount(const std::string& account_name) {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::ETH, mojom::kDefaultKeyringId, account_name);
  }

  void SetSelectedAccount(const mojom::AccountIdPtr& account_id) {
    base::RunLoop run_loop;
    keyring_service_->SetSelectedAccount(
        account_id.Clone(), base::BindLambdaForTesting([&](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void AddEthereumChain(const url::Origin& origin,
                        const std::string& chain_id) {
    mojom::NetworkInfo chain = GetTestNetworkInfo1(chain_id);
    chain.rpc_endpoints =
        std::vector<GURL>({https_server_for_rpc()->base_url()});
    auto error_message =
        json_rpc_service_->AddEthereumChainForOrigin(chain.Clone(), origin);
    if (!error_message.empty()) {
      return;
    }

    // Wait for OnAddEthereumChainRequestCompleted
    base::RunLoop run_loop_chain_request_completed;
    auto observer = std::make_unique<TestJsonRpcServiceObserver>(
        run_loop_chain_request_completed.QuitClosure());
    json_rpc_service_->AddObserver(observer->GetReceiver());
    mojo::PendingRemote<mojom::JsonRpcServiceObserver> receiver;
    mojo::MakeSelfOwnedReceiver(std::move(observer),
                                receiver.InitWithNewPipeAndPassReceiver());
    json_rpc_service_->AddEthereumChainRequestCompleted(chain_id, true);
    run_loop_chain_request_completed.Run();
  }

  void CallEthereumEnable(bool is_repeat_call = false) {
    base::RunLoop run_loop;
    auto* tab_helper =
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents());
    tab_helper->SetShowBubbleCallbackForTesting(run_loop.QuitClosure());

    ASSERT_TRUE(ExecJs(web_contents(), "ethereumEnable()"));
    if (!is_repeat_call) {
      run_loop.Run();
    }

    // The bubble should be showing at this point. If it's a repeat call then
    // the bubble should already be shown from the initial call.
    ASSERT_TRUE(tab_helper->IsShowingBubble());
  }

  void UserGrantPermission(bool granted) {
    std::string expected_address = "undefined";
    if (granted) {
      permissions::BraveWalletPermissionContext::AcceptOrCancel(
          std::vector<std::string>{default_account()->address},
          mojom::PermissionLifetimeOption::kForever, web_contents());
      expected_address = base::ToLowerASCII(default_account()->address);
    } else {
      permissions::BraveWalletPermissionContext::Cancel(web_contents());
    }
    ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
              granted);
    // Check that window.ethereum.selectedAddress is set correctly
    EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
              expected_address);
  }

  void AddEthereumPermission(const mojom::AccountIdPtr& account_id) {
    EXPECT_TRUE(permissions::BraveWalletPermissionContext::AddPermission(
        blink::PermissionType::BRAVE_ETHEREUM, browser()->profile(),
        web_contents()->GetPrimaryMainFrame()->GetLastCommittedOrigin(),
        account_id->address));
  }

  const mojom::AccountInfoPtr& default_account() const {
    return default_account_;
  }

  void ApproveTransaction(const std::string& chain_id,
                          const std::string& tx_meta_id) {
    base::RunLoop run_loop;
    tx_service_->ApproveTransaction(
        mojom::CoinType::ETH, chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](bool success,
                                       mojom::ProviderErrorUnionPtr error_union,
                                       const std::string& error_message) {
          EXPECT_TRUE(success);
          ASSERT_TRUE(error_union->is_provider_error());
          EXPECT_EQ(error_union->get_provider_error(),
                    mojom::ProviderError::kSuccess);
          EXPECT_TRUE(error_message.empty());
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void RejectTransaction(const std::string& chain_id,
                         const std::string& tx_meta_id) {
    base::RunLoop run_loop;
    tx_service_->RejectTransaction(
        mojom::CoinType::ETH, chain_id, tx_meta_id,
        base::BindLambdaForTesting([&](bool success) {
          EXPECT_TRUE(success);
          observer()->WaitForRejectedStatus();
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void WaitForSendOrSignTransactionResultReady() {
    content::DOMMessageQueue message_queue;
    std::string message;
    EXPECT_TRUE(message_queue.WaitForMessage(&message));
    EXPECT_EQ("\"result ready\"", message);
  }

  void TestUserApproved(
      std::optional<std::string> expected_signed_tx,
      const std::string& test_method,
      const std::string& data = "",
      bool skip_restore = false,
      const std::string& chain_id = mojom::kLocalhostChainId) {
    if (!skip_restore) {
      RestoreWallet();
    }
    bool sign_only = expected_signed_tx.has_value();
    GURL url = https_server_for_files()->GetURL(
        "a.com", "/send_or_sign_transaction.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_TRUE(WaitForLoadStop(web_contents()));

    CallEthereumEnable();
    UserGrantPermission(true);
    ASSERT_TRUE(ExecJs(
        web_contents(),
        absl::StrFormat(
            "sendOrSignTransaction(%s, %s, '%s', "
            "'0x084DCb94038af1715963F149079cE011C4B22961', "
            "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11', '%s');",
            sign_only ? "true" : "false",
            observer()->expect_eip1559_tx() ? "true" : "false", test_method,
            data)));
    observer()->WaitForNewUnapprovedTx();
    base::RunLoop().RunUntilIdle();
    ASSERT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());

    auto infos = GetAllTransactionInfo(chain_id);
    ASSERT_EQ(1UL, infos.size());
    EXPECT_EQ(default_account()->account_id, infos[0]->from_account_id);
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(default_account()->address,
                                                 *infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[0]->tx_status);
    EXPECT_EQ(MakeOriginInfo(https_server_for_files()->GetOrigin("a.com")),
              infos[0]->origin_info);
    ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
    EXPECT_TRUE(infos[0]
                    ->tx_data_union->get_eth_tx_data_1559()
                    ->base_data->nonce.empty());

    ApproveTransaction(chain_id, infos[0]->id);

    infos = GetAllTransactionInfo(chain_id);
    EXPECT_EQ(1UL, infos.size());
    EXPECT_EQ(default_account()->account_id, infos[0]->from_account_id);
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(default_account()->address,
                                                 *infos[0]->from_address));
    if (sign_only) {
      EXPECT_EQ(mojom::TransactionStatus::Signed, infos[0]->tx_status);
    } else {
      EXPECT_EQ(mojom::TransactionStatus::Submitted, infos[0]->tx_status);
    }
    EXPECT_FALSE(infos[0]->tx_hash.empty());
    ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
    EXPECT_EQ(infos[0]->tx_data_union->get_eth_tx_data_1559()->base_data->nonce,
              "0x9604");

    WaitForSendOrSignTransactionResultReady();
    if (sign_only) {
      EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionResult()")
                    .ExtractString(),
                *expected_signed_tx);
    } else {
      EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionResult()")
                    .ExtractString(),
                "0x00000000000009604");
    }
  }

  void TestUserRejected(
      bool sign_only,
      const std::string& test_method,
      const std::string& chain_id = mojom::kLocalhostChainId) {
    RestoreWallet();
    GURL url = https_server_for_files()->GetURL(
        "a.com", "/send_or_sign_transaction.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_TRUE(WaitForLoadStop(web_contents()));

    CallEthereumEnable();
    UserGrantPermission(true);
    ASSERT_TRUE(
        ExecJs(web_contents(),
               absl::StrFormat(
                   "sendOrSignTransaction(%s, false, '%s', "
                   "'0x084DCb94038af1715963F149079cE011C4B22961', "
                   "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');",
                   sign_only ? "true" : "false", test_method)));
    observer()->WaitForNewUnapprovedTx();
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());

    auto infos = GetAllTransactionInfo(chain_id);
    EXPECT_EQ(1UL, infos.size());
    EXPECT_EQ(default_account()->account_id, infos[0]->from_account_id);
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(default_account()->address,
                                                 *infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Unapproved, infos[0]->tx_status);
    EXPECT_EQ(MakeOriginInfo(https_server_for_files()->GetOrigin("a.com")),
              infos[0]->origin_info);
    ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
    EXPECT_TRUE(infos[0]
                    ->tx_data_union->get_eth_tx_data_1559()
                    ->base_data->nonce.empty());

    RejectTransaction(mojom::kLocalhostChainId, infos[0]->id);

    infos = GetAllTransactionInfo(chain_id);
    EXPECT_EQ(1UL, infos.size());
    EXPECT_EQ(default_account()->account_id, infos[0]->from_account_id);
    EXPECT_TRUE(base::EqualsCaseInsensitiveASCII(default_account()->address,
                                                 *infos[0]->from_address));
    EXPECT_EQ(mojom::TransactionStatus::Rejected, infos[0]->tx_status);
    EXPECT_TRUE(infos[0]->tx_hash.empty());
    ASSERT_TRUE(infos[0]->tx_data_union->is_eth_tx_data_1559());
    EXPECT_TRUE(infos[0]
                    ->tx_data_union->get_eth_tx_data_1559()
                    ->base_data->nonce.empty());

    WaitForSendOrSignTransactionResultReady();
    EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionError()")
                  .ExtractString(),
              l10n_util::GetStringUTF8(
                  IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
  }

  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo(
      const std::string& chain_id) {
    std::vector<mojom::TransactionInfoPtr> transaction_infos;
    base::RunLoop run_loop;
    tx_service_->GetAllTransactionInfo(
        mojom::CoinType::ETH, chain_id, default_account()->account_id.Clone(),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::TransactionInfoPtr> v) {
              transaction_infos = std::move(v);
              run_loop.Quit();
            }));
    run_loop.Run();
    return transaction_infos;
  }

  void TestSendTransactionError(bool sign_only,
                                const std::string& test_method) {
    RestoreWallet();
    GURL url = https_server_for_files()->GetURL(
        "a.com", "/send_or_sign_transaction.html");
    ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    EXPECT_TRUE(WaitForLoadStop(web_contents()));

    CallEthereumEnable();
    UserGrantPermission(true);
    ASSERT_TRUE(ExecJs(
        web_contents(),
        absl::StrFormat("sendOrSignTransaction(%s, false, '%s', "
                        "'0x084DCb94038af1715963F149079cE011C4B22961', "
                        "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11', "
                        "'invalid');",
                        sign_only ? "true" : "false", test_method)));

    WaitForSendOrSignTransactionResultReady();
    EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionError()")
                  .ExtractString(),
              "Internal JSON-RPC error");
  }

  void SetNetworkForTesting(const std::string& chain_id,
                            const std::optional<::url::Origin>& origin,
                            bool skip_rpc_url_override = false) {
    mojom::NetworkInfoPtr chain;
    ASSERT_TRUE(
        json_rpc_service_->SetNetwork(chain_id, mojom::CoinType::ETH, origin));
    base::RunLoop run_loop;
    json_rpc_service_->GetNetwork(
        mojom::CoinType::ETH, origin,
        base::BindLambdaForTesting([&](mojom::NetworkInfoPtr info) {
          chain = info.Clone();
          run_loop.Quit();
        }));
    run_loop.Run();
    if (chain && !skip_rpc_url_override) {
      base::RunLoop run_loop1;
      browser()->profile()->GetPrefs()->ClearPref(kBraveWalletCustomNetworks);
      chain->rpc_endpoints =
          std::vector<GURL>({https_server_for_rpc()->base_url()});
      json_rpc_service_->AddChain(
          std::move(chain),
          base::BindLambdaForTesting([&](const std::string& chain_id_out,
                                         mojom::ProviderError error,
                                         const std::string& error_message) {
            ASSERT_EQ(chain_id_out, chain_id);
            ASSERT_EQ(error, mojom::ProviderError::kSuccess);
            ASSERT_TRUE(error_message.empty());
            run_loop1.Quit();
          }));
      run_loop1.Run();
    }
    // Needed so ChainChangedEvent observers run
    base::RunLoop().RunUntilIdle();
  }

  std::string chain_id(const std::optional<::url::Origin>& origin) {
    return json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH, origin);
  }
  NetworkManager* network_manager() { return network_manager_.get(); }

 protected:
  mojom::AccountInfoPtr default_account_;

 private:
  content::ContentMockCertVerifier mock_cert_verifier_;
  TestTxServiceObserver observer_;
  base::test::ScopedFeatureList scoped_feature_list_;
  net::test_server::EmbeddedTestServer https_server_for_files_;
  net::test_server::EmbeddedTestServer https_server_for_rpc_;
  raw_ptr<NetworkManager, DanglingUntriaged> network_manager_ = nullptr;
  raw_ptr<KeyringService, DanglingUntriaged> keyring_service_ = nullptr;
  raw_ptr<TxService, DanglingUntriaged> tx_service_ = nullptr;
  raw_ptr<JsonRpcService, DanglingUntriaged> json_rpc_service_ = nullptr;
};

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedRequestSend) {
  TestUserApproved(std::nullopt, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, UserApprovedSend1) {
  TestUserApproved(std::nullopt, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, UserApprovedSend2) {
  TestUserApproved(std::nullopt, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSendAsync) {
  TestUserApproved(std::nullopt, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedRequestData0x) {
  TestUserApproved(std::nullopt, "request", "0x");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend1Data0x) {
  TestUserApproved(std::nullopt, "send1", "0x1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend2Data0x) {
  TestUserApproved(std::nullopt, "send2", "0x11");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSendAsyncData0x) {
  TestUserApproved(std::nullopt, "sendAsync", "0x");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, UserRejectedRequest) {
  TestUserRejected(false, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, UserRejectedSend1) {
  TestUserRejected(false, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, UserRejectedSend2) {
  TestUserRejected(false, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserRejectedSendAsync) {
  TestUserRejected(false, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SendTransactionErrorRequest) {
  TestSendTransactionError(false, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SendTransactionErrorSend1) {
  TestSendTransactionError(false, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SendTransactionErrorSend2) {
  TestSendTransactionError(false, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SendTransactionErrorSendAsync) {
  TestSendTransactionError(false, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedRequestSign) {
  TestUserApproved(kSignedTransaction, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend1Sign) {
  TestUserApproved(kSignedTransaction, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend2Sign) {
  TestUserApproved(kSignedTransaction, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSendAsyncSign) {
  TestUserApproved(kSignedTransaction, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedRequestData0xSign) {
  TestUserApproved(kSignedTransaction, "request", "0x");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend1Data0xSign) {
  TestUserApproved(
      "0xf8688296048525f38e9e0082960494084dcb94038af1715963f149079ce011c4b22962"
      "1101820a95a0ea3d09b65bb17424978c9ec3c9319c157523374dde70025b52034ae33f85"
      "82a8a02a879841219186d6d1029d674a6ad428e5e6693ac6b92304905fcaae533d69a3",
      "send1", "0x1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSend2Data0xSign) {
  TestUserApproved(
      "0xf8688296048525f38e9e0082960494084dcb94038af1715963f149079ce011c4b22962"
      "1111820a96a0fe7acb8944ff3223ddb123ac046129093998087d9895203cb472ed865b6a"
      "7213a071581e1fd537e114e7416322c06857f38df4e1f91e1abc9adb7cfb5840eaabca",
      "send2", "0x11");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserApprovedSendAsyncData0xSign) {
  TestUserApproved(kSignedTransaction, "sendAsync", "0x");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserRejectedRequestSign) {
  TestUserRejected(true, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserRejectedSend1Sign) {
  TestUserRejected(true, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserRejectedSend2Sign) {
  TestUserRejected(true, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       UserRejectedSendAsyncSign) {
  TestUserRejected(true, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SignTransactionErrorRequest) {
  TestSendTransactionError(true, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SignTransactionErrorSend1) {
  TestSendTransactionError(true, "send1");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SignTransactionErrorSend2) {
  TestSendTransactionError(true, "send2");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SignTransactionErrorSendAsync) {
  TestSendTransactionError(true, "sendAsync");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, InvalidAddress) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);
  for (bool sign_only : {true, false}) {
    ASSERT_TRUE(
        ExecJs(web_contents(),
               content::JsReplace(
                   "sendOrSignTransaction($1, false, 'request', "
                   "'0x6b1Bd828cF8CE051B6282dCFEf6863746E2E1909', "
                   "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');",
                   sign_only)));

    WaitForSendOrSignTransactionResultReady();
    EXPECT_FALSE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionError()")
                  .ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  }
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, NoEthPermission) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(false);
  for (bool sign_only : {true, false}) {
    ASSERT_TRUE(
        ExecJs(web_contents(),
               content::JsReplace(
                   "sendOrSignTransaction($1, false, 'request', "
                   "'0x084DCb94038af1715963F149079cE011C4B22961', "
                   "'0x084DCb94038af1715963F149079cE011C4B22962', '0x11');",
                   sign_only)));

    WaitForSendOrSignTransactionResultReady();
    EXPECT_FALSE(
        brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents())
            ->IsShowingBubble());
    EXPECT_EQ(EvalJs(web_contents(), "getSendOrSignTransactionError()")
                  .ExtractString(),
              l10n_util::GetStringUTF8(IDS_WALLET_NOT_AUTHED));
  }
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, SelectedAddress) {
  RestoreWallet();
  auto added_account = AddAccount("account 2");
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();
  UserGrantPermission(true);

  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
            base::ToLowerASCII(default_account()->address));

  // Locking the wallet makes the selectedAddress property undefined
  LockWallet();
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
            "undefined");

  // Unlock wallet restores the selectedAddress property
  UnlockWallet();
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
            base::ToLowerASCII(default_account()->address));

  // Changing the selected account doesn't change selectedAddress property
  // because it's not allowed yet.
  SetSelectedAccount(added_account->account_id);
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
            base::ToLowerASCII(default_account()->address));

  // But it does update the selectedAddress if the account is allowed
  AddEthereumPermission(added_account->account_id);
  // Wait for KeyringService::GetSelectedAccount called by
  // BraveWalletProviderDelegateImpl::GetAllowedAccounts
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(EvalJs(web_contents(), "getSelectedAddress()").ExtractString(),
            base::ToLowerASCII(added_account->address));
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, NetworkVersion) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  const auto& origin = url::Origin::Create(url);
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));
  EXPECT_EQ(EvalJs(web_contents(), "getChainId()").ExtractString(),
            chain_id(origin));
  uint256_t chain_id_uint256;
  EXPECT_TRUE(HexValueToUint256(chain_id(origin), &chain_id_uint256));
  EXPECT_EQ(EvalJs(web_contents(), "getNetworkVersion()").ExtractString(),
            base::NumberToString((uint64_t)chain_id_uint256));

  // Newly added network change
  std::string chain_id = "0x38";
  AddEthereumChain(origin, chain_id);
  SetNetworkForTesting(chain_id, origin, true);
  EXPECT_EQ(EvalJs(web_contents(), "getChainId()").ExtractString(), chain_id);
  EXPECT_TRUE(HexValueToUint256(chain_id, &chain_id_uint256));
  EXPECT_EQ(EvalJs(web_contents(), "getNetworkVersion()").ExtractString(),
            base::NumberToString((uint64_t)chain_id_uint256));

  // Make sure chainId > uint64_t has networkVersion undefined. This is
  // just a current limitation that we will likely get rid of in the future.
  chain_id = "0x878678326eac900000000";
  AddEthereumChain(origin, chain_id);
  SetNetworkForTesting(chain_id, origin, true);
  EXPECT_EQ(EvalJs(web_contents(), "getChainId()").ExtractString(), chain_id);
  EXPECT_TRUE(HexValueToUint256(chain_id, &chain_id_uint256));
  EXPECT_EQ(EvalJs(web_contents(), "getNetworkVersion()").ExtractString(),
            "undefined");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, IsUnlocked) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  EXPECT_TRUE(EvalJs(web_contents(), "getIsUnlocked()").ExtractBool());
  LockWallet();
  EXPECT_FALSE(EvalJs(web_contents(), "getIsUnlocked()").ExtractBool());
  UnlockWallet();
  EXPECT_TRUE(EvalJs(web_contents(), "getIsUnlocked()").ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, IsConnected) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));
  EXPECT_TRUE(EvalJs(web_contents(), "getIsConnected()").ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest, CallViaProxy) {
  RestoreWallet();
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));
  EXPECT_TRUE(EvalJs(web_contents(), "getIsConnectedViaProxy()").ExtractBool());
  EXPECT_TRUE(
      EvalJs(web_contents(), "getIsBraveWalletViaProxy()").ExtractBool());
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSendTransactionEIP1559Tx) {
  SetNetworkForTesting(mojom::kMainnetChainId, std::nullopt);
  observer()->SetExpectEip1559Tx(true);
  TestUserApproved(std::nullopt, "request", "", false, mojom::kMainnetChainId);
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSendTransactionLegacyTx) {
  SetNetworkForTesting(mojom::kLocalhostChainId, std::nullopt);
  observer()->SetExpectEip1559Tx(false);
  TestUserApproved(std::nullopt, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSendTransactionCustomNetworkLegacyTx) {
  RestoreWallet();

  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");
  network_manager()->AddCustomNetwork(chain);

  SetNetworkForTesting("0x5566", std::nullopt);
  observer()->SetExpectEip1559Tx(false);

  TestUserApproved(std::nullopt, "request", "", true /* skip_restore */,
                   "0x5566");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSignTransactionEIP1559Tx) {
  SetNetworkForTesting(mojom::kMainnetChainId, std::nullopt);
  observer()->SetExpectEip1559Tx(true);
  TestUserApproved(
      "0x02f86d0182960484f38e9e008525f38e9e0082960494084dcb94038af1715963f14907"
      "9ce011c4b229621180c001a0e152033adac7e7316007446c0cd45b97a21911b4e414b087"
      "2d0f207dd9ac4226a07ed1a15909a925716d97ab6e2c7077c7b4b0616c8bc522bcd4914a"
      "79ef5e6d1d",
      "request", "", false, mojom::kMainnetChainId);
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSignTransactionLegacyTx) {
  SetNetworkForTesting(mojom::kLocalhostChainId, std::nullopt);  // localhost
  observer()->SetExpectEip1559Tx(false);
  TestUserApproved(kSignedTransaction, "request");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EthSignTransactionCustomNetworkLegacyTx) {
  RestoreWallet();

  mojom::NetworkInfo chain = GetTestNetworkInfo1("0x5566");
  network_manager()->AddCustomNetwork(chain);

  SetNetworkForTesting("0x5566", std::nullopt);
  observer()->SetExpectEip1559Tx(false);

  TestUserApproved(
      "0xf8688296048525f38e9e0082960494084dcb94038af1715963f149079ce011c4b22962"
      "118082aaf0a01789b12329c3b46db7bc23af14df45ebc54f6ce8da40f4db4cec866c73bf"
      "2ed5a0642ca22062c10f05bfce787107c44001ed8bf1a1d8416cf2e9b133aadbc88076",
      "request", "", true /* skip_restore */, "0x5566");
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       SecondEnableCallFails) {
  RestoreWallet();
  AddAccount("account 2");
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();

  // 2nd call should fail
  CallEthereumEnable(/*is_repeat_call*/ true);
  ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
            false);

  // But now user should still be able to resolve the first call
  UserGrantPermission(true);
  ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
            true);
}

IN_PROC_BROWSER_TEST_F(SendOrSignTransactionBrowserTest,
                       EnableCallRequestsUnlockIfLocked) {
  RestoreWallet();
  AddAccount("account 2");
  GURL url = https_server_for_files()->GetURL("a.com",
                                              "/send_or_sign_transaction.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
  EXPECT_TRUE(WaitForLoadStop(web_contents()));

  CallEthereumEnable();

  // 2nd call should fail
  CallEthereumEnable(/*is_repeat_call*/ true);
  ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
            false);

  // But now user should still be able to resolve the first call
  UserGrantPermission(true);
  ASSERT_EQ(EvalJs(web_contents(), "getPermissionGranted()").ExtractBool(),
            true);
}

}  // namespace brave_wallet
