/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/wallet_button_notification_source.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class WalletButtonNotificationSourceTest : public InProcessBrowserTest {
 public:
  WalletButtonNotificationSourceTest() = default;

  void SetUpOnMainThread() override {
    json_rpc_service()->SetGasPriceForTesting("0x123");
    WaitForTxStorageInitialized(tx_service()->GetTxStorageForTesting());

    StartRPCServer(
        base::BindRepeating(&WalletButtonNotificationSourceTest::HandleRequest,
                            base::Unretained(this)));
  }

  ~WalletButtonNotificationSourceTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);

    command_line->AppendSwitchASCII(
        network::switches::kHostResolverRules,
        "MAP * " + https_server_for_rpc_.host_port_pair().ToString());
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

  BraveWalletService* brave_wallet_service() {
    return BraveWalletServiceFactory::GetServiceForContext(
        browser()->GetProfile());
  }

  KeyringService* keyring_service() {
    return brave_wallet_service()->keyring_service();
  }

  JsonRpcService* json_rpc_service() {
    return brave_wallet_service()->json_rpc_service();
  }

  TxService* tx_service() { return brave_wallet_service()->tx_service(); }

  AccountUtils GetAccountUtils() { return AccountUtils(keyring_service()); }

  void CreateWallet() {
    GetAccountUtils().CreateWallet(kMnemonicDripCaution, kTestWalletPassword);
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/json");

    auto body = base::test::ParseJsonDict(request.content);
    auto* method = body.FindString("method");
    if (!method) {
      ADD_FAILURE() << request.content;
      return nullptr;
    }
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
    https_server_for_rpc()->RegisterRequestHandler(callback);
    ASSERT_TRUE(https_server_for_rpc()->Start());
  }

  net::EmbeddedTestServer* https_server_for_rpc() {
    return &https_server_for_rpc_;
  }

 private:
  net::test_server::EmbeddedTestServer https_server_for_rpc_;
  content::ContentMockCertVerifier mock_cert_verifier_;
};

// Init() and MarkWalletButtonWasClicked() notify synchronously, so the tests
// below check the results right after those calls. Mojo observer
// notifications (wallet creation, tx changes, tx service reset) arrive
// asynchronously and are awaited with base::test::RunUntil().

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       ShowBadge_WhenWalletNotCreated) {
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, true);
  EXPECT_EQ(count_result, 0u);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       DontShowBadge_WhenWalletCreated) {
  CreateWallet();

  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, false);
  EXPECT_EQ(count_result, 0u);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletButtonClicked) {
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, true);
  EXPECT_EQ(count_result, 0u);

  notification_source->MarkWalletButtonWasClicked();

  EXPECT_EQ(show_badge_suggest_result, false);
  EXPECT_EQ(count_result, 0u);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       HideBadge_WhenWalletCreated) {
  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, true);
  EXPECT_EQ(count_result, 0u);

  CreateWallet();

  // The wallet created notification arrives over mojo.
  ASSERT_TRUE(
      base::test::RunUntil([&] { return show_badge_suggest_result == false; }));
  EXPECT_EQ(count_result, 0u);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       PendingTransactionsCounter) {
  CreateWallet();

  // Add initial FIL transaction
  std::string first_tx_meta_id;
  {
    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto fil_tx_data = mojom::FilTxData::New(
        "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
        "100" /* gas_limit */, "" /* max_fee */, to_account, "11");
    EXPECT_EQ(from_account->account_id->unique_key,
              "461_3_0_t17otcil7bookogjy3ywoslq5gf5tbisdkcfui2iq");

    base::test::TestFuture<bool, std::string, std::string> add_future;
    tx_service()->AddUnapprovedFilecoinTransaction(
        std::move(fil_tx_data), mojom::kFilecoinTestnet,
        from_account->account_id.Clone(), nullptr,
        add_future.GetCallback<bool, const std::string&, const std::string&>());
    auto [success, id, err_message] = add_future.Take();
    first_tx_meta_id = id;
    EXPECT_TRUE(success) << err_message;
  }

  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, false);
  EXPECT_EQ(count_result, 1u);

  // Add second ETH transaction
  std::string second_tx_meta_id;
  {
    const auto from_account = GetAccountUtils().EnsureEthAccount(0);
    const std::string to_account = "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c";

    auto params = mojom::NewEvmTransactionParams::New(
        "0x06", from_account->account_id.Clone(),
        "0xbe862ad9abfe6f22bcb087716c7d89a26051f74c", "0x016345785d8a0000", "",
        std::vector<uint8_t>(), nullptr);
    base::test::TestFuture<bool, std::string, std::string> add_future;
    tx_service()->AddUnapprovedEvmTransaction(
        std::move(params),
        add_future.GetCallback<bool, const std::string&, const std::string&>());
    auto [success, id, err_message] = add_future.Take();
    second_tx_meta_id = id;
    EXPECT_TRUE(success) << err_message;
  }

  // Add third SOL transaction
  std::string third_tx_meta_id;
  {
    auto from_account = GetAccountUtils().EnsureSolAccount(0);
    std::string to_account = "JDqrvDz8d8tFCADashbUKQDKfJZFobNy13ugN65t1wvV";
    const std::vector<uint8_t> data = {2,   0, 0, 0, 128, 150,
                                       152, 0, 0, 0, 0,   0};

    std::vector<mojom::SolanaAccountMetaPtr> account_metas;
    auto account_meta1 = mojom::SolanaAccountMeta::New(from_account->address,
                                                       nullptr, true, true);
    auto account_meta2 =
        mojom::SolanaAccountMeta::New(to_account, nullptr, false, true);
    account_metas.push_back(std::move(account_meta1));
    account_metas.push_back(std::move(account_meta2));

    auto instruction = mojom::SolanaInstruction::New(
        mojom::kSolanaSystemProgramId, std::move(account_metas), data, nullptr);
    std::vector<mojom::SolanaInstructionPtr> instructions;
    instructions.push_back(std::move(instruction));
    auto tx_data = mojom::SolanaTxData::New(
        "", 0, from_account->address, to_account, "", 10000000, 0,
        mojom::TransactionType::SolanaSystemTransfer, std::move(instructions),
        mojom::SolanaMessageVersion::kLegacy,
        mojom::SolanaMessageHeader::New(1, 0, 1),
        std::vector<std::string>(
            {from_account->address, to_account, mojom::kSolanaSystemProgramId}),
        std::vector<mojom::SolanaMessageAddressTableLookupPtr>(), nullptr,
        nullptr, nullptr);

    base::test::TestFuture<bool, std::string, std::string> add_future;
    tx_service()->AddUnapprovedSolanaTransaction(
        std::move(tx_data), brave_wallet::mojom::kSolanaMainnet,
        from_account->account_id.Clone(), nullptr,
        add_future.GetCallback<bool, const std::string&, const std::string&>());
    auto [success, id, err_message] = add_future.Take();
    third_tx_meta_id = id;
    EXPECT_TRUE(success) << err_message;
  }

  // New unapproved tx notifications arrive over mojo.
  ASSERT_TRUE(base::test::RunUntil([&] { return count_result == 3u; }));
  EXPECT_EQ(show_badge_suggest_result, false);

  // Reject first transaction
  {
    base::test::TestFuture<bool> reject_future;
    tx_service()->RejectTransaction(mojom::CoinType::FIL,
                                    mojom::kFilecoinMainnet, first_tx_meta_id,
                                    reject_future.GetCallback());
    EXPECT_TRUE(reject_future.Get());
  }

  ASSERT_TRUE(base::test::RunUntil([&] { return count_result == 2u; }));
  EXPECT_EQ(show_badge_suggest_result, false);

  // Reject second transaction
  {
    base::test::TestFuture<bool> reject_future;
    tx_service()->RejectTransaction(mojom::CoinType::ETH,
                                    mojom::kMainnetChainId, second_tx_meta_id,
                                    reject_future.GetCallback());
    EXPECT_TRUE(reject_future.Get());
  }

  ASSERT_TRUE(base::test::RunUntil([&] { return count_result == 1u; }));
  EXPECT_EQ(show_badge_suggest_result, false);

  // Reject third transaction
  {
    base::test::TestFuture<bool> reject_future;
    tx_service()->RejectTransaction(mojom::CoinType::SOL, mojom::kSolanaMainnet,
                                    third_tx_meta_id,
                                    reject_future.GetCallback());
    EXPECT_TRUE(reject_future.Get());
  }

  ASSERT_TRUE(base::test::RunUntil([&] { return count_result == 0u; }));
  EXPECT_EQ(show_badge_suggest_result, false);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       CounterReset_WhenResetTxService) {
  CreateWallet();

  // Add initial transaction
  {
    const auto from_account = GetAccountUtils().EnsureFilTestAccount(0);
    const std::string to_account = "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy";
    auto fil_tx_data = mojom::FilTxData::New(
        "" /* nonce */, "10" /* gas_premium */, "10" /* gas_fee_cap */,
        "100" /* gas_limit */, "" /* max_fee */, to_account, "11");
    base::test::TestFuture<bool, std::string, std::string> add_future;
    tx_service()->AddUnapprovedFilecoinTransaction(
        std::move(fil_tx_data), mojom::kFilecoinTestnet,
        from_account->account_id.Clone(), nullptr,
        add_future.GetCallback<bool, const std::string&, const std::string&>());
    auto [success, id, err_message] = add_future.Take();
    EXPECT_TRUE(success) << err_message;
  }

  std::optional<bool> show_badge_suggest_result;
  std::optional<size_t> count_result;
  auto notification_source = std::make_unique<WalletButtonNotificationSource>(
      browser()->GetProfile(), base::BindRepeating(base::BindLambdaForTesting(
                                   [&show_badge_suggest_result, &count_result](
                                       bool show_badge_suggest, size_t count) {
                                     show_badge_suggest_result =
                                         show_badge_suggest;
                                     count_result = count;
                                   })));
  notification_source->Init();

  EXPECT_EQ(show_badge_suggest_result, false);
  EXPECT_EQ(count_result, 1u);

  tx_service()->Reset();

  // The tx service reset notification arrives over mojo.
  ASSERT_TRUE(base::test::RunUntil([&] { return count_result == 0u; }));
  EXPECT_EQ(show_badge_suggest_result, false);
}

IN_PROC_BROWSER_TEST_F(WalletButtonNotificationSourceTest,
                       EmptyBadgeNotShownAfterRestart_IfClicked) {
  {
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->GetProfile(),
        base::BindRepeating(base::BindLambdaForTesting(
            [&show_badge_suggest_result, &count_result](bool show_badge_suggest,
                                                        size_t count) {
              show_badge_suggest_result = show_badge_suggest;
              count_result = count;
            })));
    notification_source->Init();

    EXPECT_EQ(show_badge_suggest_result, true);
    notification_source->MarkWalletButtonWasClicked();
  }

  {
    std::optional<bool> show_badge_suggest_result;
    std::optional<size_t> count_result;
    auto notification_source = std::make_unique<WalletButtonNotificationSource>(
        browser()->GetProfile(),
        base::BindRepeating(base::BindLambdaForTesting(
            [&show_badge_suggest_result, &count_result](bool show_badge_suggest,
                                                        size_t count) {
              show_badge_suggest_result = show_badge_suggest;
              count_result = count;
            })));
    notification_source->Init();

    EXPECT_EQ(show_badge_suggest_result, false);
  }
}

}  // namespace brave_wallet
