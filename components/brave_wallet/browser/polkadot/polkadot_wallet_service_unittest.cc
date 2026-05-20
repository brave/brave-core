/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/functional/callback_forward.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

// Use the BOB account here:
// https://westend.subscan.io/account/5FHneW46xGXgs5mUiveU4sbTyGBzmstUspZC92UhjJM694ty
inline constexpr const char kBob[] =
    "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48";
inline constexpr char kAssetHubMnemonic[] =
    "lazy february across turn unique syrup gasp pass pelican achieve cable "
    "canal";

void AddValidMetadataResponses(
    network::TestURLLoaderFactory& url_loader_factory,
    const std::string& testnet_url,
    const std::string& mainnet_url) {
  url_loader_factory.AddResponse(
      testnet_url, ReadMetadataFixtureJson("state_getMetadata_westend.json"));
  url_loader_factory.AddResponse(
      mainnet_url, ReadMetadataFixtureJson("state_getMetadata_polkadot.json"));
}

}  // namespace

class PolkadotWalletServiceUnitTest : public testing::Test {
 public:
  using TransferAll = PolkadotWalletService::TransferAll;

  PolkadotWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~PolkadotWalletServiceUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeatureWithParameters(
        brave_wallet::features::kBraveWalletPolkadotFeature,
        {{brave_wallet::features::kPolkadotParachainsEnabled.name, "true"}});

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    Init();
  }

  void Init() {
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);

    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
    polkadot_testnet_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 0);
    polkadot_mainnet_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotMainnet, 0);
    ASSERT_TRUE(polkadot_testnet_account_);
    ASSERT_TRUE(polkadot_mainnet_account_);
  }

  void UnlockWallet() {
    keyring_service_->Unlock(
        kTestWalletPassword,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, bool unlocked) {
              EXPECT_TRUE(unlocked);
              quit_closure.Run();
            },
            task_environment_.QuitClosure()));

    task_environment_.RunUntilQuit();

    keyring_service_
        ->GetKeyring<PolkadotKeyring>(mojom::KeyringId::kPolkadotTestnet)
        ->SetMockRndSeedForTesting();

    keyring_service_
        ->GetKeyring<PolkadotKeyring>(mojom::KeyringId::kPolkadotMainnet)
        ->SetMockRndSeedForTesting();
  }

  void SetPolkadotMockRndSeed(mojom::KeyringId keyring_id, uint64_t seed) {
    keyring_service_->GetKeyring<PolkadotKeyring>(keyring_id)
        ->SetMockRndSeedForTesting(seed);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

 protected:
  base::test::TaskEnvironment task_environment_;

  base::test::ScopedFeatureList feature_list_;
  mojom::AccountInfoPtr polkadot_testnet_account_;
  mojom::AccountInfoPtr polkadot_mainnet_account_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;

  network::TestURLLoaderFactory url_loader_factory_;

  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
};

void VerifyTestnet(const PolkadotChainMetadata& metadata) {
  EXPECT_EQ(metadata->balances_pallet_index, 4u);
  EXPECT_EQ(metadata->ss58_prefix, 42u);
}

void VerifyMainnet(const PolkadotChainMetadata& metadata) {
  EXPECT_EQ(metadata->balances_pallet_index, 5u);
  EXPECT_EQ(metadata->ss58_prefix, 0u);
}

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
        *keyring_service_, *network_manager_, prefs_,
        url_loader_factory_.GetSafeWeakWrapper());

    UnlockWallet();
    EXPECT_EQ(url_loader_factory_.NumPending(), 2);

    AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

    // Do this twice to prove our fetching and caching layers work.
    for (int i = 0; i < 2; ++i) {
      // Testnet chain metadata.

      polkadot_wallet_service->GetChainMetadata(
          mojom::kPolkadotTestnet,
          base::BindOnce(
              [](base::RepeatingClosure quit_closure,
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_TRUE(metadata.has_value());
                VerifyTestnet(*metadata);
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
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_TRUE(metadata.has_value());
                VerifyMainnet(*metadata);
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
        *keyring_service_, *network_manager_, prefs_,
        url_loader_factory_.GetSafeWeakWrapper());

    UnlockWallet();
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
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
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
                 base::expected<PolkadotChainMetadata, std::string> metadata) {
                EXPECT_FALSE(metadata.has_value());
                std::move(quit_closure).Run();
              },
              task_environment_.QuitClosure()));

      task_environment_.RunUntilQuit();
    }
  }
}

TEST_F(PolkadotWalletServiceUnitTest, GetChainMetadataInvalidChainId) {
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>>
      future;
  polkadot_wallet_service->GetChainMetadata("unknown-chain-id",
                                            future.GetCallback());

  auto metadata = future.Take();
  EXPECT_FALSE(metadata.has_value());
  EXPECT_EQ(metadata.error(), WalletInternalErrorMessage());
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);
  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto req_json = RequestBodyToJsonDict(req);
        const auto* method = req_json.FindString("method");
        if (!method || *method != "state_getMetadata") {
          return;
        }

        self->url_loader_factory_.ClearResponses();
        const std::string fixture_name = req.url.spec() == mainnet_url
                                             ? "state_getMetadata_polkadot.json"
                                             : "state_getMetadata_westend.json";
        self->url_loader_factory_.AddResponse(
            req.url.spec(), ReadMetadataFixtureJson(fixture_name));
      }));

  constexpr int kNumRequests = 5;
  int pending_requests = kNumRequests;
  for (int i = 0; i < kNumRequests; ++i) {
    // Testnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotTestnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               base::expected<PolkadotChainMetadata, std::string> metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              VerifyTestnet(*metadata);
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &pending_requests));
  }
  task_environment_.RunUntilQuit();

  pending_requests = kNumRequests;
  for (int i = 0; i < kNumRequests; ++i) {
    // Mainnet chain metadata.
    polkadot_wallet_service->GetChainMetadata(
        mojom::kPolkadotMainnet,
        base::BindOnce(
            [](base::RepeatingClosure quit_closure, int* num_reqs,
               base::expected<PolkadotChainMetadata, std::string> metadata) {
              EXPECT_TRUE(*num_reqs > 0);
              EXPECT_TRUE(metadata.has_value());
              VerifyMainnet(*metadata);
              if (--*num_reqs == 0) {
                std::move(quit_closure).Run();
              }
            },
            task_environment_.QuitClosure(), &pending_requests));
  }
  task_environment_.RunUntilQuit();
}

TEST_F(PolkadotWalletServiceUnitTest, GetCompatibleNetworks) {
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());
  // Start from a known visibility state for deterministic assertions.
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                        mojom::kPolkadotMainnet);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                        mojom::kPolkadotMainnetAssetHub);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                        mojom::kPolkadotTestnet);

  // Compatible networks for mainnet account.
  {
    base::test::TestFuture<std::optional<std::vector<mojom::NetworkInfoPtr>>>
        future;
    polkadot_wallet_service->GetCompatibleNetworks(
        polkadot_mainnet_account_->account_id.Clone(), future.GetCallback());

    auto networks = future.Take();
    ASSERT_TRUE(networks.has_value());
    ASSERT_EQ(networks->size(), 2u);
    EXPECT_EQ(networks->at(0)->chain_id, mojom::kPolkadotMainnet);
    EXPECT_EQ(networks->at(0)->coin, mojom::CoinType::DOT);
    EXPECT_EQ(networks->at(1)->chain_id, mojom::kPolkadotMainnetAssetHub);
    EXPECT_EQ(networks->at(1)->coin, mojom::CoinType::DOT);
  }

  // Compatible networks list changes.
  {
    network_manager_->AddHiddenNetwork(mojom::CoinType::DOT,
                                       mojom::kPolkadotMainnet);
    network_manager_->AddHiddenNetwork(mojom::CoinType::DOT,
                                       mojom::kPolkadotMainnetAssetHub);

    base::test::TestFuture<std::optional<std::vector<mojom::NetworkInfoPtr>>>
        future;
    polkadot_wallet_service->GetCompatibleNetworks(
        polkadot_mainnet_account_->account_id.Clone(), future.GetCallback());

    auto networks = future.Take();
    ASSERT_TRUE(networks.has_value());
    EXPECT_EQ(networks->size(), 0u);
  }

  // Compatible networks for testnet account.
  {
    base::test::TestFuture<std::optional<std::vector<mojom::NetworkInfoPtr>>>
        future;
    polkadot_wallet_service->GetCompatibleNetworks(
        polkadot_testnet_account_->account_id.Clone(), future.GetCallback());

    auto networks = future.Take();
    ASSERT_TRUE(networks.has_value());
    EXPECT_EQ(networks->size(), 1u);
    EXPECT_EQ((*networks)[0]->chain_id, mojom::kPolkadotTestnet);
    EXPECT_EQ((*networks)[0]->coin, mojom::CoinType::DOT);
  }
}

TEST_F(PolkadotWalletServiceUnitTest, GetAddress) {
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

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
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  // Mainnet address.
  {
    base::test::TestFuture<const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    polkadot_wallet_service->GetAddress(
        polkadot_mainnet_account_->account_id.Clone(), mojom::kPolkadotMainnet,
        future.GetCallback());

    auto [address, error] = future.Take();
    EXPECT_TRUE(address.has_value());
    EXPECT_FALSE(error.has_value());
    EXPECT_EQ(*address, "1UC3h7uQVraXVhGhfPqmk6F7syCLrxMSVbJBuQqW7R8SHdK");
  }

  // Unknown chain id.
  {
    base::test::TestFuture<const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    polkadot_wallet_service->GetAddress(
        polkadot_testnet_account_->account_id.Clone(), "asd",
        future.GetCallback());

    auto [address, error] = future.Take();
    EXPECT_FALSE(address.has_value());
    EXPECT_EQ(error, WalletInternalErrorMessage());
  }

  // Testnet.
  {
    base::test::TestFuture<const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    polkadot_wallet_service->GetAddress(
        polkadot_testnet_account_->account_id.Clone(), mojom::kPolkadotTestnet,
        future.GetCallback());

    auto [address, error] = future.Take();
    EXPECT_TRUE(address.has_value());
    EXPECT_EQ(*address, "5CXtuMrqYib75xgkk2LqdbG6GFyYeZQDMzrp2cRUx2PcFzsA");
  }

  // Address\chain_id mismatch.
  {
    network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                          mojom::kPolkadotTestnet);

    base::test::TestFuture<const std::optional<std::string>&,
                           const std::optional<std::string>&>
        future;
    polkadot_wallet_service->GetAddress(
        polkadot_mainnet_account_->account_id.Clone(), mojom::kPolkadotTestnet,
        future.GetCallback());

    auto [address, error] = future.Take();
    EXPECT_FALSE(address.has_value());
    EXPECT_EQ(error, WalletInternalErrorMessage());
  }
}

TEST_F(PolkadotWalletServiceUnitTest, ValidateAddressForTransaction) {
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

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

  // Invalid chain_id should fail before metadata fetch.
  {
    base::test::TestFuture<mojom::PolkadotValidationStatus> future;
    polkadot_wallet_service->ValidateAddressForTransaction(
        "unknown-chain-id", "158HHeYTmEXMiMM1XufQt5bEe2CTia3EcVcfrpYBYcXA6bdb",
        future.GetCallback());
    EXPECT_EQ(future.Get(),
              mojom::PolkadotValidationStatus::kInvalidAddressFormat);
  }

  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  {
    base::test::TestFuture<mojom::PolkadotValidationStatus> future;
    polkadot_wallet_service->ValidateAddressForTransaction(
        mojom::kPolkadotMainnet,
        "158HHeYTmEXMiMM1XufQt5bEe2CTia3EcVcfrpYBYcXA6bdb",
        future.GetCallback());
    EXPECT_EQ(future.Get(), mojom::PolkadotValidationStatus::kNoError);
  }

  {
    base::test::TestFuture<mojom::PolkadotValidationStatus> future;
    polkadot_wallet_service->ValidateAddressForTransaction(
        mojom::kPolkadotMainnet,
        "5GvDB3LMJCoBVPyf7KgbfLe17FG7aQq2qqBKQ2YW9rJqNpHS",
        future.GetCallback());
    EXPECT_EQ(future.Get(), mojom::PolkadotValidationStatus::kInvalidPrefix);
  }

  {
    base::test::TestFuture<mojom::PolkadotValidationStatus> future;
    polkadot_wallet_service->ValidateAddressForTransaction(
        mojom::kPolkadotMainnet, "not-an-address", future.GetCallback());
    EXPECT_EQ(future.Get(),
              mojom::PolkadotValidationStatus::kInvalidAddressFormat);
  }
}

TEST_F(PolkadotWalletServiceUnitTest,
       ValidateAddressForTransaction_MetadataFetchFails) {
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  std::string mainnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  url_loader_factory_.ClearResponses();
  url_loader_factory_.AddResponse(mainnet_url, R"(
        { "jsonrpc": "2.0",
          "result": "invalid-metadata",
          "id": 1 })");

  base::test::TestFuture<mojom::PolkadotValidationStatus> future;
  polkadot_wallet_service->ValidateAddressForTransaction(
      mojom::kPolkadotMainnet,
      "158HHeYTmEXMiMM1XufQt5bEe2CTia3EcVcfrpYBYcXA6bdb", future.GetCallback());
  EXPECT_EQ(future.Get(),
            mojom::PolkadotValidationStatus::kFailedToFetchMetadata);
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "result":{
                "specName":"westend",
                "implName":"parity-westend",
                "authoringVersion":2,
                "specVersion":1021000,
                "implVersion":0,
                "apis":[
                  ["0xdf6acb689907609b",5],["0x37e397fc7c91f5e4",2],["0xccd9de6396c899ca",1],["0x40fe3ad401f8959a",6],
                  ["0xd2bc9897eed08f15",3],["0xf78b278be53f454c",2],["0xaf2c0297a23e6d3d",15],["0x49eaaf1b548a0cb0",6],
                  ["0x91d5df18b0d2cf58",3],["0x2a5e924655399e60",1],["0xed99c5acb25eedf5",3],["0xcbca25e39f142387",2],
                  ["0x687ad44ad37f03c2",1],["0xab3c0572291feb8b",1],["0xbc9d89904f5b923f",1],["0x37c8bb1350a9a2a8",4],
                  ["0xf3ff14d5ab527059",3],["0x6ff52ee858e6c5bd",2],["0x91b1c8b16328eb92",2],["0x9ffb505aa738d69c",1],
                  ["0x17a6bc0d0062aeb3",1],["0x18ef58a3b67ba770",1],["0xfbc577b9d747efd6",1],["0x2609be83ac4468dc",1]
                ],
                "transactionVersion":27,
                "systemVersion":1,
                "stateVersion":1
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        } else {
          NOTREACHED() << req_json;
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  // clang-format off
  constexpr static std::string_view kExpectedExtrinsic =
      "3502"  // Length prefix.
      "84"    // 0x80 => signed, 0x04 => extrinsic version v4
      "00"    // Address type.
      "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e" // Sender pubkey.
      "01"  // Signature type (0x01 => Sr25519)
      "1a847b30e2d9a658d52234e673056933258552bb107e533788a072663624d002" // Signature
      "ba2e25b63be097d31acd35eb49a09994f09a4450e94559f380ea7f787e30d982"
      "5501"    // Mortal era.
      "440000"  // Nonce is 17, SCALE-encoded as 0x44; tip = 0; mode = 0.
      "0403"    // Pallet index, call index.
      "00"      // Address type
      "8eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a48" // Recipient address
      "4913"  // Send amount.
      ;
  // clang-format on

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  ASSERT_TRUE(signed_extrinsic.has_value());
  EXPECT_EQ(base::HexEncodeLower(signed_extrinsic->extrinsic()),
            kExpectedExtrinsic);
  EXPECT_EQ(base::HexEncodeLower(signed_extrinsic->block_hash()),
            "46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21");
  EXPECT_EQ(signed_extrinsic->block_num(), 0x1c06355u);
  EXPECT_EQ(signed_extrinsic->mortality_period(), 64u);
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoChainMetadata) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // Keep metadata responses stable across constructor warmup and the explicit
  // request below, including init retries.
  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        auto req_json = RequestBodyToJsonDict(req);
        const auto* method = req_json.FindString("method");
        if (!method || *method != "state_getMetadata") {
          return;
        }

        self->url_loader_factory_.ClearResponses();
        if (req.url.spec() == testnet_url) {
          self->url_loader_factory_.AddResponse(req.url.spec(), R"(
            { "jsonrpc": "2.0",
              "error": {
                "code": 1234
              },
              "id": 1 })");
          return;
        }

        self->url_loader_factory_.AddResponse(
            req.url.spec(),
            ReadMetadataFixtureJson("state_getMetadata_polkadot.json"));
      }));

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoAccountInfo) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "error": {
                "code": 1234
              }
            })");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoChainHeader) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "error": {
                "code": 1234
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoParentHeader) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "error":{
                "code": 1234
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoFinalizedHead) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"error":{"code": 1234}})");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest,
       SignTransferExtrinsic_NoFinalizedBlockHeader) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "error":{
                "code": 1234
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoGenesisHash) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;

  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "result":{
                "specName":"westend",
                "implName":"parity-westend",
                "authoringVersion":2,
                "specVersion":1021000,
                "implVersion":0,
                "apis":[
                  ["0xdf6acb689907609b",5],["0x37e397fc7c91f5e4",2],["0xccd9de6396c899ca",1],["0x40fe3ad401f8959a",6],
                  ["0xd2bc9897eed08f15",3],["0xf78b278be53f454c",2],["0xaf2c0297a23e6d3d",15],["0x49eaaf1b548a0cb0",6],
                  ["0x91d5df18b0d2cf58",3],["0x2a5e924655399e60",1],["0xed99c5acb25eedf5",3],["0xcbca25e39f142387",2],
                  ["0x687ad44ad37f03c2",1],["0xab3c0572291feb8b",1],["0xbc9d89904f5b923f",1],["0x37c8bb1350a9a2a8",4],
                  ["0xf3ff14d5ab527059",3],["0x6ff52ee858e6c5bd",2],["0x91b1c8b16328eb92",2],["0x9ffb505aa738d69c",1],
                  ["0x17a6bc0d0062aeb3",1],["0x18ef58a3b67ba770",1],["0xfbc577b9d747efd6",1],["0x2609be83ac4468dc",1]
                ],
                "transactionVersion":27,
                "systemVersion":1,
                "stateVersion":1
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"error":{"code":1234}})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoRuntimeVersion) {
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
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();
  EXPECT_EQ(url_loader_factory_.NumPending(), 2);

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  // For the two initial network calls that fetch the chain metadata.
  AddValidMetadataResponses(url_loader_factory_, testnet_url, mainnet_url);

  url_loader_factory_.ClearResponses();

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  // Our initial call to get account information so we have a usable nonce for
  // extinsic creation.
  req_res_pairs.emplace(base::test::ParseJsonDict(
                            R"(
            {
              "id":1,
              "jsonrpc":"2.0",
              "method":"state_queryStorageAt",
              "params":[
                [
                  "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9993172CE9066ECAD0B20E568F0DB6A2314BCCFBAD15C6327408E833D162271F93A51FA3A6BC67D3EACC384BB9704D71E"
                ]
              ]
            })"),
                        R"(
            {
              "jsonrpc":"2.0",
              "id":8,
              "result":[
                {
                  "block":"0xdcc9741f0258ede18a1684ff787c1591db8585f3154481cd162378fdd6677056",
                  "changes":[
                    [
                      "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9c6282eb06d1994674b75538b85244e4952707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c",
                      "0x11000000000000000100000000000000be2bcb22d90800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
                    ]
                  ]
                }
              ]
            })");

  // Our initial call to get the most recent block header in the chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":10,
              "result": {
                "parentHash": "0x5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2",
                "number": "0x1c06358",
                "stateRoot": "0xc7ce642fe3c31724fc2808e179d807e5b0ba38a40eb5c08d98a41bd22eeeb9b5",
                "extrinsicsRoot": "0xba8c3f44dbde4e14831d61cb349f808f902bf4474112cf45c2c7d92bef4e754e",
                "digest": {
                  "logs": [
                    "0x0642414245b501030d0000002a92911100000000641ad61d0d848c4b374270406d092e8e5ca294e78ab6ea47d6da8ffe51e207758c49a32cac0d5d1e1ca56e4996b2136f42602debdf22478aff56b359adeadf06912cba0095e5b5abc3d7c0e03d9b628587010e6b44c6f9f40419b16bec4f5204",
                    "0x04424545468403b61c769585188ea959dc10c9434bfa46d57e818c7f17e047c75c42b3b1389c11",
                    "0x0542414245010186630051f53fb7ce6e02d1e020383357327c9e28ebfc399a96a98ad42528290a3f57cd65de0688f232da0b9db4a5007e69da55e2e13146c4f5c60983fc9c138e"
                  ]
                }
              }
            })");

  // Chained call from grabbing the most recent block header in the chain. Grab
  // the header of the corresponding parent hash for mortality calculations,
  // like how polkadot-js's api package does.
  // Implement the same algorithm as here:
  // https://github.com/polkadot-js/api/blob/eb34741c871ca8d029a9706ae989ba8ce865db0f/packages/api-derive/src/tx/signingInfo.ts#L41-L71
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["5834828e919dc0eccce83080104cc14f51f81330451bdf74bbc9bc1edba618f2"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":12,
              "result":{
                "parentHash":"0x08cf134277f266acbce9727a13e7675ef472eb5932c693d7e924a5e12ca89b66",
                "number":"0x1c06357",
                "stateRoot":"0x6ac80fab8d74177b34675338269aad0d19d61dbe9adc2f421f77742bc4153d2d",
                "extrinsicsRoot":"0x2ee277dc4f23c4b3ab4e84cd7bb060cceca658933aed2735699e6d434cea39bc",
                "digest":{
                  "logs":[
                    "0x0642414245b50103000000002992911100000000d0053711418b7b2b943547844945a8eba27aa6163575a5b3aadd7cb78c878f6e2c8f4af3018fd5ad87d36dcca26914a9324722334311c8055e3889db61d5e60fa2eb601a422f67f1daf0a08d434c151430d1467855699ddb294e347e5ecdf90e",
                    "0x04424545468403ffb231bc483aba17682e4b1968b90559cab35ee078c021899f6751b2fc845085",
                    "0x05424142450101407020be76e604fdbb36bc1cd07dc71aa9eeddd537c26fec73178e48effde4607c728bb10f06c94299af0cb1a5835bb60bab5d9c3cc6eff8a0ade2a9a13f1780"
                  ]
                }
              }
            })");

  // Our initial call to get the hash of the last finalized block in the canon
  // chain.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":11,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Chained call, grab the block header using the hash of the finalized head.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":13,
              "result":{
                "parentHash":"0xcf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4",
                "number":"0x1c06355",
                "stateRoot":"0x3a501ddbfc394d859401cd6d55f5743461ddb3a5aecfebb31f587c16ad23f505",
                "extrinsicsRoot":"0x8fc47b641e793ed938eae4d793636b2feb657bca97726a43ee3375a8e5b321a6",
                "digest":{
                  "logs":[
                    "0x0642414245b501030200000027929111000000008038b165beaf68d4ae8b7a3eae2055ecdfde0a0462993a43e522c709773da51a550d604eb90a671b88437f7f0d5e7f2e4efe323e2cee3992ffa2bcd3e5e10d07ff37c43e11e82263d2bc774942196e96c05a38bbbd820eff1cbf2441b2c59307",
                    "0x04424545468403cfdc267eac55b3225fe8d581f3d2f7d9ece28a564bb70b50dd04b829e893b78a",
                    "0x05424142450101fc0b1a7fcff42ffb1fcb8166843fb9b9eded36f64891deea28eea90da9215e70c605638b274f0c8517fc70d0c2b1442fd50ad933ee6cf7ceba600f762e2bd682"
                  ]
                }
              }
            })");

  // Grab the block hash of whichever block header we're using for signing the
  // extrinsic. The polkadot-js algorithm selects between either the finalized
  // head or the parent of the current head. In this case, the finalized block
  // wound up winning out. Theoretically, this can be replaced by just directly
  // hashing the block headers locally.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["01C06355"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x46e5afe42b1ff0c40ecc18d7ff97974f3bdf5dfda1e21d779644a7ea30a97d21"})");

  // Grab the runtime version of whichever block we're going to use for signing.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf424e463b14b26905d4e2aaff455a3c149c3ccff5a1fc62203c0c07b711e3f4"]})"),
      R"(
            {
              "jsonrpc":"2.0",
              "id":15,
              "error":{
                "code":1234
              }
            })");

  // We need to grab the genesis block hash.
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"})");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          const std::string fixture_name =
              req.url.spec() == mainnet_url ? "state_getMetadata_polkadot.json"
                                            : "state_getMetadata_westend.json";
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(fixture_name));
          return;
        }
        auto pos = req_res_pairs.find(req_json);

        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(testnet_url, pos->second);
        }
      }));

  auto pubkey = base::HexEncodeLower(
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value());

  EXPECT_EQ(pubkey,
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  base::test::TestFuture<base::expected<PolkadotExtrinsicMetadata, std::string>>
      test_future;

  polkadot_wallet_service->GenerateSignedTransferExtrinsic(
      mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      test_future.GetCallback());

  auto signed_extrinsic = test_future.Take();
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignAndSendTransaction) {
  // Test the normal happy path where we create a signed extrinsic for the
  // specified account and then author it on the block chain.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(polkadot_mainnet_account_->account_id)
          .value();

  static constexpr char kExpectedExtrinsic[] =
      R"(3502840014bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e0172b76b135defb247cfc61436fdb4aea6b62620565acd1d7ba65424f5ab3b3348a82d1cbe64d61b4523c397509713645c445fc4135f2fc2d2a936ab0f8ba0458155014400000503008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a488543)";

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->SetExpectedExtrinsic(kExpectedExtrinsic);
  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  std::string chain_id = mojom::kPolkadotMainnet;

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  polkadot_wallet_service->SignAndSendTransaction(
      chain_id, polkadot_mainnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{4321}), recipient_pubkey,
      future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_TRUE(tx_hash.has_value());
  EXPECT_EQ(
      tx_hash->first,
      "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a423968d");
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.extrinsic()),
            kExpectedExtrinsic);
}

TEST_F(PolkadotWalletServiceUnitTest, SignAndSendTransaction_WestendAssetHub) {
  // Captured transaction:
  // https://assethub-westend.subscan.io/extrinsic/0xcc92467cebaee29feb3a8eba5ec1314f903cd69e6b0ab93832900737251f42db
  keyring_service_->Reset();
  GetAccountUtils().CreateWallet(kAssetHubMnemonic, kTestWalletPassword);

  auto sender =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 0);
  auto recipient =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 1);
  ASSERT_TRUE(sender);
  ASSERT_TRUE(recipient);
  SetPolkadotMockRndSeed(mojom::KeyringId::kPolkadotTestnet, 127);

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(sender->account_id).value();
  EXPECT_EQ(base::HexEncodeLower(sender_pubkey),
            "0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
            "881b7e71e");

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  ASSERT_TRUE(base::HexStringToSpan(
      "ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e",
      recipient_pubkey));
  EXPECT_EQ(
      base::HexEncodeLower(
          keyring_service_->GetPolkadotPubKey(recipient->account_id).value()),
      base::HexEncodeLower(recipient_pubkey));

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  const std::string westend_asset_hub_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnetAssetHub, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  static constexpr char kExpectedExtrinsic[] =
      "4d0284000e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
      "881b7e71e01821f988e7ca41ef24164bb2a1b948fcd9a2e97cd08f58ac5a898"
      "4cabfc83596670890d4df3dd15878b7282369aaa996261a05429e7f172c41c3d"
      "8c753981b88cb502040000000a0300ae70948d0c015b6c2b1ac46b8931ad630"
      "1f2c648f3f0adf71d08a68fe745561e0b00409452a303";

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  req_res_pairs.emplace(base::test::ParseJsonDict(R"({
        "id":1,
        "jsonrpc":"2.0",
        "method":"state_queryStorageAt",
        "params":[[
          "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA96F72E0A390DB2406281323AC697D46F10E161E17289C260A07020CC2A23192E882D5BEE006B1390DEED844B881B7E71E"
        ]]
      })"),
                        R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":[{
          "block":"0xa3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470",
          "changes":[[
            "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da96f72e0a390db2406281323ac697d46f10e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e",
            "0x010000000000000001000000000000002549373a460700000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
          ]]
        }]
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xa3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470",
          "number":"0xe812ec",
          "stateRoot":"0x660fd3729bf44973d4a0d8e7fe8c7641e5ee75f06e07938328f40f3360404e95",
          "extrinsicsRoot":"0x6683b881be64726d41b7046208424987636461d6c2d27fafd1ef27305c208ecf",
          "digest":{"logs":[
            "0x06434d4c53100101010c",
            "0x06434d4c530c020001",
            "0x066175726120be576b0400000000",
            "0x045250535290bef1467f9f2659ebdc4b394b1eb32dd468706c031b74dc1377221d782b00ea6296b87007",
            "0x056175726101017642ed848bf7839d8d522c3f13808bb24365679e13674cb75ff820757d5b57126af41c54d7afa758c09820c0879d44e039366ee6132d2f35bcc6f01d7e2d4986"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0xd5c74ee2e5347f396b637f3b25bfed717ceb533798fa5c470c626b09245dc4ca"})");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x67f9723393ef76214df0118c34bbbd3dbebc8ed46a10973a8c969d48fe7598c9"})");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["a3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xcf7164032bad56873d6753e8c7c3a99232699204e680d793ab3172cdf64c2bb3",
          "number":"0xe812eb",
          "stateRoot":"0x605dec0661e9d051dee09953449cbbb60f0b2476cb867b1a6b869d6aad2eb3eb",
          "extrinsicsRoot":"0xb8bdaf98b502f3d36120a24d3cdcf60af96e6ac1c10f7a9d36bce3e272ac6722",
          "digest":{"logs":[
            "0x06434d4c53100100010c",
            "0x06434d4c530c020001",
            "0x066175726120be576b0400000000",
            "0x045250535290bef1467f9f2659ebdc4b394b1eb32dd468706c031b74dc1377221d782b00ea6296b87007",
            "0x05617572610101f23a5412e25899a1820896ddf2b2889162911bafe77933758267624cf137c334d1adb71555a9d6ee2086f6d02901b269d9228853b3bc2e53b643313534474187"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["d5c74ee2e5347f396b637f3b25bfed717ceb533798fa5c470c626b09245dc4ca"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xcd5d0b2d2ebb6c07e930e57d13aa86c145bc03545508283e403e3e8c230b7416",
          "number":"0xe812dc",
          "stateRoot":"0x4b6049a34903b556c6289baa3d087bd5907972c89d12bc83fa82e347a0761f59",
          "extrinsicsRoot":"0xf5bfd271857b1b5053f4945d151aec4a5361c8d6fb7c731e3c1e590b4979837a",
          "digest":{"logs":[
            "0x06434d4c53100100010c",
            "0x06434d4c530c020001",
            "0x066175726120bd576b0400000000",
            "0x0452505352900580d4ec4ea12acf8aba8911ec5e81d96715aa74ecb955253a89386c7e770f4182b87007",
            "0x0561757261010154e8737cf43c00597d6771589b3d0577f4f16ab4dc983ee0deb5c99e4be04751ebc8d7ae4d81924e2eaeb993ec21dc7d9222cf837e5e93c28a707f29a3371b88"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["cf7164032bad56873d6753e8c7c3a99232699204e680d793ab3172cdf64c2bb3"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "specName":"westmint",
          "implName":"westmint",
          "authoringVersion":1,
          "specVersion":1022006,
          "implVersion":0,
          "apis":[["0x40fe3ad401f8959a",6]],
          "transactionVersion":16,
          "systemVersion":1,
          "stateVersion":1
        }
      })");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();
        EXPECT_EQ(req.url.spec(), westend_asset_hub_url);

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(
                                  "state_getMetadata_assethub_westend.json"));
          return;
        }

        if (const auto* method = req_json.FindString("method");
            method && *method == "author_submitExtrinsic") {
          const auto* params = req_json.FindList("params");
          ASSERT_TRUE(params);
          ASSERT_EQ(params->size(), 1u);
          EXPECT_EQ((*params)[0].GetString(), kExpectedExtrinsic);
          self->url_loader_factory_.AddResponse(req.url.spec(), R"({
            "jsonrpc":"2.0",
            "id":1,
            "result":"0xcc92467cebaee29feb3a8eba5ec1314f903cd69e6b0ab93832900737251f42db"
          })");
          return;
        }

        auto pos = req_res_pairs.find(req_json);
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(req.url.spec(), pos->second);
        } else {
          NOTREACHED() << req_json;
        }
      }));

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  polkadot_wallet_service->SignAndSendTransaction(
      mojom::kPolkadotTestnetAssetHub, sender->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{4000000000000ull}),
      recipient_pubkey, future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_TRUE(tx_hash.has_value());
  EXPECT_EQ(
      tx_hash->first,
      "0xcc92467cebaee29feb3a8eba5ec1314f903cd69e6b0ab93832900737251f42db");
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.extrinsic()),
            kExpectedExtrinsic);
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.block_hash()),
            "a3a6ef09932ad0a6086e0900431d09a9488e3e905e277cf7bb5a8ed8bfa90470");
  EXPECT_EQ(tx_hash->second.block_num(), 0xe812ebu);
  EXPECT_EQ(tx_hash->second.mortality_period(), 64u);
}

TEST_F(PolkadotWalletServiceUnitTest, SignAndSendTransaction_PolkadotAssetHub) {
  // Captured transaction:
  // https://assethub-polkadot.subscan.io/extrinsic/0x32cf17114363bc9b26256104cab96d19a1052b4407452e05709fb6876d64daff
  keyring_service_->Reset();
  GetAccountUtils().CreateWallet(kAssetHubMnemonic, kTestWalletPassword);

  auto sender =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotMainnet, 0);
  auto recipient =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotMainnet, 1);
  ASSERT_TRUE(sender);
  ASSERT_TRUE(recipient);
  SetPolkadotMockRndSeed(mojom::KeyringId::kPolkadotMainnet, 127);

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(sender->account_id).value();
  EXPECT_EQ(base::HexEncodeLower(sender_pubkey),
            "0e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
            "881b7e71e");

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  ASSERT_TRUE(base::HexStringToSpan(
      "ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e",
      recipient_pubkey));
  EXPECT_EQ(
      base::HexEncodeLower(
          keyring_service_->GetPolkadotPubKey(recipient->account_id).value()),
      base::HexEncodeLower(recipient_pubkey));

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  const std::string polkadot_asset_hub_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotMainnetAssetHub, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  static constexpr char kExpectedExtrinsic[] =
      "490284000e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
      "881b7e71e019ab2d49ce64d7dd15a6b0beab4dbd71b29f5b661c2b98867e473"
      "18c7402d770133cd398d8ca7a1238d2a8e50142ac5d705a267e7eeab82ae146a"
      "4045b96256845501040000000a0300ae70948d0c015b6c2b1ac46b8931ad630"
      "1f2c648f3f0adf71d08a68fe745561e0700e40b5402";

  base::flat_map<base::DictValue, std::string_view> req_res_pairs;
  req_res_pairs.emplace(base::test::ParseJsonDict(R"({
        "id":1,
        "jsonrpc":"2.0",
        "method":"state_queryStorageAt",
        "params":[[
          "0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA96F72E0A390DB2406281323AC697D46F10E161E17289C260A07020CC2A23192E882D5BEE006B1390DEED844B881B7E71E"
        ]]
      })"),
                        R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":[{
          "block":"0xe89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3",
          "changes":[[
            "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da96f72e0a390db2406281323ac697d46f10e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e",
            "0x01000000000000000100000000000000bdd98fa7040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
          ]]
        }]
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":[]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xe89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3",
          "number":"0xf56f96",
          "stateRoot":"0xdfdef333dc49f9083ea3bc8f075476d4a72c5f34596c10ef81cd83026e1153e9",
          "extrinsicsRoot":"0x4005234663e7885ea59d6e1a21f41e6a55e7c363098778c8d966d6d9f4e876c3",
          "digest":{"logs":[
            "0x06434d4c53100101010c",
            "0x06617572612028b1d60800000000",
            "0x0452505352900552a633f21315079c06500c78a0b198c2fec6b776d2e5a89fb31629b83a1e75522f7907",
            "0x05617572610101e3ff2ff746c0fe5ae99e7c66d05a99b6ebe79b5316e1796ac21f2a276b9672fc7348e77fbf6b8cf63368cc0ca0c5b593490c5af31361b61262244bb42393820e"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getFinalizedHead","params":[]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x64fc8fc096c5ae976c484d4cf1d0ab0ab45ba7386185db73c2a25852135da861"})");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getBlockHash","params":["00000000"]})"),
      R"({"jsonrpc":"2.0","id":1,"result":"0x68d56f15f85d3136970ec16946040bc1752654e906147f7e43e9d539d7c3de2f"})");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["e89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0x910f058dca1089e202af72fa36a1f218b9807aefb2804845deec476c7df76101",
          "number":"0xf56f95",
          "stateRoot":"0x627c70115ad17fe48b12203e5c4f64741da2c4066b9229af893af4c1ce5866fb",
          "extrinsicsRoot":"0xa6da7aaaf9f88a7d405c2305fbfd33948cbe3d46f919f1abfaba10818e795689",
          "digest":{"logs":[
            "0x06434d4c53100100010c",
            "0x06617572612028b1d60800000000",
            "0x0452505352900552a633f21315079c06500c78a0b198c2fec6b776d2e5a89fb31629b83a1e75522f7907",
            "0x05617572610101698bb38428085853a155742448491abb966497763e77f51717165da2430a895ffeca232bbb72449adaa77a0dbc53a56dd27a8f03ee8f76abc9042493c5627707"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"chain_getHeader","params":["64fc8fc096c5ae976c484d4cf1d0ab0ab45ba7386185db73c2a25852135da861"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "parentHash":"0xa4b930e78d719ecd8b1fe7188d252a64125be73be8caf8bf79a905f039890164",
          "number":"0xf56f87",
          "stateRoot":"0x2f7207c00e4bb61a780f8826f2acdbc71a542be73b894cf5cb05ac100477b075",
          "extrinsicsRoot":"0xf33f960ece73152b37f52adbd4fad63242ed640d6644325348b174d4a1346e86",
          "digest":{"logs":[
            "0x06434d4c53100101010c",
            "0x06617572612025b1d60800000000",
            "0x04525053529013b127db899f033548408932652dcabd2739e46499b041a96feeb89b0f918a173e2f7907",
            "0x05617572610101cb41ac1516773fda2511bec42139b7a23287e8d7699eb2bfb63d870831fe143ad03bf2744fed53fcfa992ecd9a32d21d0399864f76fd9f0e22d8103500b93201"
          ]}
        }
      })");
  req_res_pairs.emplace(
      base::test::ParseJsonDict(
          R"({"id":1,"jsonrpc":"2.0","method":"state_getRuntimeVersion","params":["910f058dca1089e202af72fa36a1f218b9807aefb2804845deec476c7df76101"]})"),
      R"({
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "specName":"statemint",
          "implName":"statemint",
          "authoringVersion":1,
          "specVersion":2002001,
          "implVersion":0,
          "apis":[["0xbc9d89904f5b923f",1]],
          "transactionVersion":15,
          "systemVersion":1,
          "stateVersion":1
        }
      })");

  url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&, self = this](const network::ResourceRequest& req) {
        self->url_loader_factory_.ClearResponses();
        EXPECT_EQ(req.url.spec(), polkadot_asset_hub_url);

        auto req_json = RequestBodyToJsonDict(req);
        if (const auto* method = req_json.FindString("method");
            method && *method == "state_getMetadata") {
          self->url_loader_factory_.AddResponse(
              req.url.spec(), ReadMetadataFixtureJson(
                                  "state_getMetadata_assethub_polkadot.json"));
          return;
        }

        if (const auto* method = req_json.FindString("method");
            method && *method == "author_submitExtrinsic") {
          const auto* params = req_json.FindList("params");
          ASSERT_TRUE(params);
          ASSERT_EQ(params->size(), 1u);
          EXPECT_EQ((*params)[0].GetString(), kExpectedExtrinsic);
          self->url_loader_factory_.AddResponse(req.url.spec(), R"({
            "jsonrpc":"2.0",
            "id":1,
            "result":"0x32cf17114363bc9b26256104cab96d19a1052b4407452e05709fb6876d64daff"
          })");
          return;
        }

        auto pos = req_res_pairs.find(req_json);
        if (pos != req_res_pairs.end()) {
          self->url_loader_factory_.AddResponse(req.url.spec(), pos->second);
        } else {
          NOTREACHED() << req_json;
        }
      }));

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  polkadot_wallet_service->SignAndSendTransaction(
      mojom::kPolkadotMainnetAssetHub, sender->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{10000000000ull}),
      recipient_pubkey, future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_TRUE(tx_hash.has_value());
  EXPECT_EQ(
      tx_hash->first,
      "0x32cf17114363bc9b26256104cab96d19a1052b4407452e05709fb6876d64daff");
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.extrinsic()),
            kExpectedExtrinsic);
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.block_hash()),
            "e89e79113fca59edd36bc3cc84fbb4e73516575fc396c94495350c42773c83e3");
  EXPECT_EQ(tx_hash->second.block_num(), 0xf56f95u);
  EXPECT_EQ(tx_hash->second.mortality_period(), 64u);
}

TEST_F(PolkadotWalletServiceUnitTest, SignAndSendTransaction_TransferAll) {
  // Test the normal happy path, but this time for transfer_all instead of our
  // normal transfer_keep_alive.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(polkadot_mainnet_account_->account_id)
          .value();

  static constexpr char kExpectedExtrinsic[] =
      R"(3102840014bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e01fc2d6274dafecb46f06887e0a00e68b4ce7b5db9a05c56b55603c958a40ecd077b0bef114157efe399ee1ad40eb45fc2c0a44ff77ce381eb99233b9aa62f608455014400000504008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4800)";

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->SetExpectedExtrinsic(kExpectedExtrinsic);
  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  std::string chain_id = mojom::kPolkadotMainnet;

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  polkadot_wallet_service->SignAndSendTransaction(
      chain_id, polkadot_mainnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(TransferAll{}), recipient_pubkey,
      future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_TRUE(tx_hash.has_value());
  EXPECT_EQ(
      tx_hash->first,
      "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a423968d");
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.extrinsic()),
            kExpectedExtrinsic);
}

TEST_F(PolkadotWalletServiceUnitTest,
       SignAndSendTransaction_InvalidSubmitExtrinsic) {
  // Test that we correctly see an error if the RPC nodes reject the extrinsic.

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(polkadot_mainnet_account_->account_id)
          .value();

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->RejectExtrinsicSubmission();
  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  std::string chain_id = mojom::kPolkadotMainnet;

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  polkadot_wallet_service->SignAndSendTransaction(
      chain_id, polkadot_mainnet_account_->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{4321}), recipient_pubkey,
      future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_FALSE(tx_hash.has_value());
  EXPECT_EQ(tx_hash.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, GetFeeEstimate) {
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  // We opt into using a manual RunLoop here because of the limitations around
  // base::test::TestFuture and uint128_t.
  base::RunLoop run_loop;
  auto quit = run_loop.QuitClosure();

  std::string chain_id = mojom::kPolkadotTestnet;

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  polkadot_wallet_service->GetFeeEstimate(
      chain_id, polkadot_testnet_account_->account_id->Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      base::BindLambdaForTesting(
          [=](base::expected<uint128_t, std::string> partial_fee) {
            ASSERT_TRUE(partial_fee.has_value());
            EXPECT_EQ(partial_fee.value(), uint128_t{15937408476ull});
            quit.Run();
          }));

  run_loop.Run();
}

TEST_F(PolkadotWalletServiceUnitTest, GetFeeEstimate_NetworkFailure) {
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  polkadot_mock_rpc->RejectAccountInfoRequest();
  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  // We opt into using a manual RunLoop here because of the limitations around
  // base::test::TestFuture and uint128_t.
  base::RunLoop run_loop;
  auto quit = run_loop.QuitClosure();

  std::string chain_id = mojom::kPolkadotTestnet;

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  polkadot_wallet_service->GetFeeEstimate(
      chain_id, polkadot_testnet_account_->account_id->Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
      base::BindLambdaForTesting(
          [=](base::expected<uint128_t, std::string> partial_fee) {
            ASSERT_FALSE(partial_fee.has_value());
            quit.Run();
          }));

  run_loop.Run();
}

}  // namespace brave_wallet
