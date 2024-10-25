/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <memory>
#include <optional>

#include "base/test/bind.h"
#include "base/test/values_test_util.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

class WalletButtonNotificationSourceTest : public InProcessBrowserTest {
 public:
  WalletButtonNotificationSourceTest() = default;

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");

    auto* wallet_service =
        brave_wallet::BraveWalletServiceFactory::GetServiceForContext(
            browser()->profile());
    network_manager_ = wallet_service->network_manager();
    json_rpc_service_ = wallet_service->json_rpc_service();
    json_rpc_service_->SetGasPriceForTesting("0x123");
    keyring_service_ = wallet_service->keyring_service();
    tx_service_ = wallet_service->tx_service();
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());

    StartRPCServer(
        base::BindRepeating(&WalletButtonNotificationSourceTest::HandleRequest,
                            base::Unretained(this)));
  }

  ~WalletButtonNotificationSourceTest() override = default;

  brave_wallet::NetworkManager* network_manager() { return network_manager_; }
  brave_wallet::TxService* tx_service() { return tx_service_; }
  brave_wallet::KeyringService* keyring_service() { return keyring_service_; }

  brave_wallet::AccountUtils GetAccountUtils() {
    return brave_wallet::AccountUtils(keyring_service_);
  }

  void CreateWallet() {
    GetAccountUtils().CreateWallet(brave_wallet::kMnemonicDripCaution,
                                   brave_wallet::kTestWalletPassword);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");
    std::string request_path = request.GetURL().path();

    auto body = base::test::ParseJsonDict(request.content);
    auto* method = body.FindString("method");
    EXPECT_TRUE(method);
    std::string reply;

    if (*method == "getBlockHeight") {
      reply = R"({"jsonrpc":"2.0","id":1,"result":18446744073709551615})";
    } else if (*method == "getLatestBlockhash") {
      reply = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "context": {
            "slot": 1069
          },
          "value": {
            "blockhash": "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N",
            "lastValidBlockHeight": 18446744073709551615
          }
        }
      })";
    } else if (*method == "simulateTransaction") {
      reply = R"({
        "jsonrpc": "2.0",
        "result": {
          "context": {
            "apiVersion": "1.17.25",
            "slot": 259225005
          },
          "value": {
            "accounts": null,
            "err": null,
            "logs": [
              "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
              "Program log: Instruction: Transfer",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV invoke [2]",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV consumed 39 of 183791 compute units",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV success",
              "Program cmtDvXumGCrqC1Age74AVPhSRVXJMd8PJS91L8KbNCK invoke [2]",
              "Program log: Instruction: ReplaceLeaf",
              "Program log: Attempting to fill in proof",
              "Program consumption: 148976 units remaining",
              "Program log: Active Index: 4",
              "Program log: Rightmost Index: 1479308",
              "Program log: Buffer Size: 64",
              "Program log: Leaf Index: 885106",
              "Program log: Fast-forwarding proof, starting index 4",
              "Program consumption: 145902 units remaining",
              "Program consumption: 145795 units remaining",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV invoke [3]",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV consumed 39 of 133311 compute units",
              "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV success",
              "Program cmtDvXumGCrqC1Age74AVPhSRVXJMd8PJS91L8KbNCK consumed 36402 of 168927 compute units",
              "Program cmtDvXumGCrqC1Age74AVPhSRVXJMd8PJS91L8KbNCK success",
              "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY consumed 69017 of 200000 compute units",
              "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY success"
            ],
            "returnData": null,
            "unitsConsumed": 69017
          }
        },
        "id": 1
      })";
    } else if (*method == "getSignatureStatuses") {
      reply = R"("")";
    } else if (*method == "getFeeForMessage") {
      reply = R"({"jsonrpc":"2.0", "id":1, "result":{"value":5000}})";
    } else if (*method == "getRecentPrioritizationFees") {
      reply = R"({
        "jsonrpc": "2.0",
        "result": [
          {
            "prioritizationFee": 100,
            "slot": 293251906
          },
          {
            "prioritizationFee": 200,
            "slot": 293251906
          },
          {
            "prioritizationFee": 0,
            "slot": 293251805
          }
        ],
        "id": 1
      })";
    } else {
      reply = "";
    }
    http_response->set_content(reply);
    return std::move(http_response);
  }

  void StartRPCServer(
      const net::EmbeddedTestServer::HandleRequestCallback& callback) {
    https_server_for_rpc()->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    https_server_for_rpc()->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_for_rpc()->Start());

    // Update rpc url for kLocalhostChainId
    brave_wallet::mojom::NetworkInfoPtr chain;
    json_rpc_service_->SetNetwork(brave_wallet::mojom::kLocalhostChainId,
                                  brave_wallet::mojom::CoinType::SOL,
                                  std::nullopt);
    base::RunLoop run_loop;
    json_rpc_service_->GetNetwork(
        brave_wallet::mojom::CoinType::SOL, std::nullopt,
        base::BindLambdaForTesting(
            [&](brave_wallet::mojom::NetworkInfoPtr info) {
              chain = info.Clone();
              run_loop.Quit();
            }));
    run_loop.Run();
    base::RunLoop run_loop1;
    chain->rpc_endpoints =
        std::vector<GURL>({https_server_for_rpc()->base_url()});
    json_rpc_service_->AddChain(
        std::move(chain),
        base::BindLambdaForTesting([&](const std::string& chain_id,
                                       brave_wallet::mojom::ProviderError error,
                                       const std::string& error_message) {
          ASSERT_EQ(chain_id, brave_wallet::mojom::kLocalhostChainId);
          ASSERT_EQ(error, brave_wallet::mojom::ProviderError::kSuccess);
          ASSERT_TRUE(error_message.empty());
          run_loop1.Quit();
        }));
    run_loop1.Run();
  }

  net::EmbeddedTestServer* https_server_for_rpc() {
    return &https_server_for_rpc_;
  }

 private:
  raw_ptr<brave_wallet::NetworkManager, DanglingUntriaged> network_manager_;
  raw_ptr<brave_wallet::KeyringService, DanglingUntriaged> keyring_service_;
  raw_ptr<brave_wallet::TxService, DanglingUntriaged> tx_service_;
  raw_ptr<brave_wallet::JsonRpcService, DanglingUntriaged> json_rpc_service_ =
      nullptr;
  net::test_server::EmbeddedTestServer https_server_for_rpc_;
};

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       ShowBadge_WhenWalletNotCreated) {
  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       DontShowBadge_WhenWalletCreated) {
  CreateWallet();

  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletButtonClicked) {
  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());

  notification_source->MarkWalletButtonWasClicked();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletCreated) {
  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_TRUE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());

  CreateWallet();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       PendingTransactionsCounter) {
  CreateWallet();

  // Add initial FIL transaction
  std::string first_tx_meta_id;
  {
    base::RunLoop run_loop;

    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, "11"));
    auto chain_id = network_manager()->GetCurrentChainId(
        brave_wallet::mojom::CoinType::FIL, std::nullopt);
    EXPECT_EQ(chain_id, "t");
    EXPECT_EQ(from_account->account_id->unique_key,
              "461_3_0_t17otcil7bookogjy3ywoslq5gf5tbisdkcfui2iq");

    tx_service()->AddUnapprovedTransaction(
        std::move(tx_data), chain_id, from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          first_tx_meta_id = id;
          EXPECT_TRUE(success) << err_message;
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop idle_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Add second ETH transaction
  std::string second_tx_meta_id;
  {
    base::RunLoop run_loop;

    const auto from_account = GetAccountUtils().EnsureEthAccount(0);
    const std::string to_account = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";

    auto params = brave_wallet::mojom::NewEvmTransactionParams::New(
        "0x06", from_account->account_id.Clone(),
        "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000", "",
        std::vector<uint8_t>());
    tx_service()->AddUnapprovedEvmTransaction(
        std::move(params),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          second_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  // Add third SOL transaction
  std::string third_tx_meta_id;
  {
    base::RunLoop run_loop;

    auto from_account = GetAccountUtils().EnsureSolAccount(0);
    std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
    const std::vector<uint8_t> data = {2,   0, 0, 0, 128, 150,
                                       152, 0, 0, 0, 0,   0};

    std::vector<brave_wallet::mojom::SolanaAccountMetaPtr> account_metas;
    auto account_meta1 = brave_wallet::mojom::SolanaAccountMeta::New(
        from_account->address, nullptr, true, true);
    auto account_meta2 = brave_wallet::mojom::SolanaAccountMeta::New(
        to_account, nullptr, false, true);
    account_metas.push_back(std::move(account_meta1));
    account_metas.push_back(std::move(account_meta2));

    auto instruction = brave_wallet::mojom::SolanaInstruction::New(
        brave_wallet::mojom::kSolanaSystemProgramId, std::move(account_metas),
        data, nullptr);
    std::vector<brave_wallet::mojom::SolanaInstructionPtr> instructions;
    instructions.push_back(std::move(instruction));
    auto tx_data = brave_wallet::mojom::SolanaTxData::New(
        "", 0, from_account->address, to_account, "", 10000000, 0,
        brave_wallet::mojom::TransactionType::SolanaSystemTransfer,
        std::move(instructions),
        brave_wallet::mojom::SolanaMessageVersion::kLegacy,
        brave_wallet::mojom::SolanaMessageHeader::New(1, 0, 1),
        std::vector<std::string>({from_account->address, to_account,
                                  brave_wallet::mojom::kSolanaSystemProgramId}),
        std::vector<brave_wallet::mojom::SolanaMessageAddressTableLookupPtr>(),
        nullptr, nullptr, nullptr);

    tx_service()->AddUnapprovedTransaction(
        brave_wallet::mojom::TxDataUnion::NewSolanaTxData(std::move(tx_data)),
        network_manager()->GetCurrentChainId(brave_wallet::mojom::CoinType::SOL,
                                             std::nullopt),
        from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          third_tx_meta_id = id;
          EXPECT_TRUE(success);
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(3u, count_result.value());

  // Reject first transaction
  {
    base::RunLoop run_loop;
    tx_service()->RejectTransaction(
        brave_wallet::mojom::CoinType::FIL,
        network_manager()->GetCurrentChainId(brave_wallet::mojom::CoinType::FIL,
                                             std::nullopt),
        first_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(2u, count_result.value());

  // Reject second transaction
  {
    base::RunLoop run_loop;
    tx_service()->RejectTransaction(
        brave_wallet::mojom::CoinType::ETH,
        network_manager()->GetCurrentChainId(brave_wallet::mojom::CoinType::ETH,
                                             std::nullopt),
        second_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  // Reject third transaction
  {
    base::RunLoop run_loop;
    tx_service()->RejectTransaction(
        brave_wallet::mojom::CoinType::SOL,
        network_manager()->GetCurrentChainId(brave_wallet::mojom::CoinType::SOL,
                                             std::nullopt),
        third_tx_meta_id, base::BindLambdaForTesting([&](bool result) {
          EXPECT_TRUE(result);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  // Wait until WalletButtonNotificationSource checks are finished
  idle_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       CounterReset_WhenResetTxService) {
  CreateWallet();

  // Add initial transaction
  std::string tx_meta_id;
  {
    base::RunLoop run_loop;

    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto tx_data = brave_wallet::mojom::TxDataUnion::NewFilTxData(
        brave_wallet::mojom::FilTxData::New(
            "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
            "100" /* gas_limit */, "" /* max_fee */, to_account, "11"));
    auto chain_id = network_manager()->GetCurrentChainId(
        brave_wallet::mojom::CoinType::FIL, std::nullopt);
    EXPECT_EQ(chain_id, "t");
    tx_service()->AddUnapprovedTransaction(
        std::move(tx_data), chain_id, from_account->account_id.Clone(),
        base::BindLambdaForTesting([&](bool success, const std::string& id,
                                       const std::string& err_message) {
          tx_meta_id = id;
          EXPECT_TRUE(success) << err_message;
          run_loop.Quit();
        }));

    run_loop.Run();
  }

  base::RunLoop run_loop;
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                [&show_badge_suggest_result, &count_result](
                                    bool show_badge_suggest, size_t count) {
                                  show_badge_suggest_result =
                                      show_badge_suggest;
                                  count_result = count;
                                })));
  notification_source->Init();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(1u, count_result.value());

  tx_service()->Reset();

  // Wait until WalletButtonNotificationSource checks are finished
  run_loop.RunUntilIdle();

  EXPECT_FALSE(show_badge_suggest_result.value());
  EXPECT_EQ(0u, count_result.value());
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       EmptyBadgeNotShownAfterRestart_IfClicked) {
  {
    base::RunLoop run_loop;
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                  [&show_badge_suggest_result, &count_result](
                                      bool show_badge_suggest, size_t count) {
                                    show_badge_suggest_result =
                                        show_badge_suggest;
                                    count_result = count;
                                  })));
    notification_source->Init();
    run_loop.RunUntilIdle();

    EXPECT_TRUE(show_badge_suggest_result.value());
    notification_source->MarkWalletButtonWasClicked();
  }

  {
    base::RunLoop run_loop;
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->profile(), base::BindRepeating(base::BindLambdaForTesting(
                                  [&show_badge_suggest_result, &count_result](
                                      bool show_badge_suggest, size_t count) {
                                    show_badge_suggest_result =
                                        show_badge_suggest;
                                    count_result = count;
                                  })));
    notification_source->Init();
    run_loop.RunUntilIdle();

    EXPECT_FALSE(show_badge_suggest_result.value());
  }
}

}  // namespace brave
