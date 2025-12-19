/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/functional/callback_forward.h"
#include "base/test/task_environment.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class PolkadotWalletServiceUnitTest : public testing::Test {
 public:
  PolkadotWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~PolkadotWalletServiceUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, shared_url_loader_factory_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
};

TEST_F(PolkadotWalletServiceUnitTest, Constructor) {
  // Basic Hello, World style test for getting chain data from the constructor
  // calls.

  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  {
    // Both requests in the constructor complete successfully.

    auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);

    EXPECT_EQ(url_loader_factory_.NumPending(), 2);

    url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

    url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

    // Do this twice to prove our fetching and caching layers work.
    for (int i = 0; i < 2; ++i) {
      // Testnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotTestnet,
          base::BindOnce(
              [](base::RepeatingClosure quit_closure,
                 const base::expected<PolkadotChainMetadata, std::string>&
                     metadata) {
                EXPECT_TRUE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }

    // Do this twice to prove our fetching and caching layers work.
    for (int i = 0; i < 2; ++i) {
      // Mainnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotMainnet,
          base::BindOnce(
              [](base::RepeatingClosure quit_closure,
                 const base::expected<PolkadotChainMetadata, std::string>&
                     metadata) {
                EXPECT_TRUE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }
  }

  url_loader_factory_.ClearResponses();

  {
    // Both responses are invalid.

    auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
        *keyring_service_, *network_manager_, shared_url_loader_factory_);

    EXPECT_EQ(url_loader_factory_.NumPending(), 2);

    url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "hello",
      "id": 1 })");

    url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "world",
      "id": 1 })");

    {
      // Testnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotTestnet,
          base::BindOnce(
              [](base::RepeatingCallback<void()> quit_closure,
                 const base::expected<PolkadotChainMetadata, std::string>&
                     metadata) {
                EXPECT_FALSE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }

    {
      // Mainnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotMainnet,
          base::BindOnce(
              [](base::RepeatingCallback<void()> quit_closure,
                 const base::expected<PolkadotChainMetadata, std::string>&
                     metadata) {
                EXPECT_FALSE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }
  }
}

TEST_F(PolkadotWalletServiceUnitTest, ConcurrentChainNameFetches) {
  // Test callback caching for getting chain names.

  url_loader_factory_.ClearResponses();

  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");
  EXPECT_EQ(mainnet_url, "https://polkadot-mainnet.wallet.brave.com/");

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, shared_url_loader_factory_);

  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");

  url_loader_factory_.AddResponse(mainnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Polkadot",
      "id": 1 })");

  int num_requests = 5;
  for (int i = 0; i < num_requests; ++i) {
    // Testnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotTestnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               const base::expected<PolkadotChainMetadata, std::string>&
                   metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &num_requests));
  }
  task_environment_.RunUntilQuit();

  num_requests = 5;
  for (int i = 0; i < num_requests; ++i) {
    // Mainnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotMainnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               const base::expected<PolkadotChainMetadata, std::string>&
                   metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &num_requests));
  }
  task_environment_.RunUntilQuit();
}

}  // namespace brave_wallet
