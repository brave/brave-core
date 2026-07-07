/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "base/functional/callback_forward.h"
#include "base/functional/function_ref.h"
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
    feature_list_.InitAndEnableFeature(
        brave_wallet::features::kBraveWalletPolkadotFeature);

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

  // Drives GenerateSignedTransferExtrinsic on testnet against a mock RPC whose
  // signing steps all succeed except for the one disabled by `configure` (e.g.
  // PolkadotMockRpc::RejectInitialChainHeader). Returns the result, which the
  // caller asserts is an error.
  base::expected<PolkadotExtrinsicMetadata, std::string>
  SignTransferExtrinsicWithFailedStep(
      base::FunctionRef<void(PolkadotMockRpc&)> configure) {
    PolkadotMockRpc polkadot_mock_rpc(&url_loader_factory_,
                                      network_manager_.get());
    PolkadotWalletService polkadot_wallet_service(
        *keyring_service_, *network_manager_, prefs_,
        url_loader_factory_.GetSafeWeakWrapper());

    UnlockWallet();

    auto sender_pubkey =
        keyring_service_
            ->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
            .value();

    polkadot_mock_rpc.SetSenderPubKey(sender_pubkey);
    configure(polkadot_mock_rpc);
    polkadot_mock_rpc.AddReqResPairs();
    polkadot_mock_rpc.FinalizeSetup();

    std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
    EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

    base::test::TestFuture<
        base::expected<PolkadotExtrinsicMetadata, std::string>>
        test_future;
    polkadot_wallet_service.GenerateSignedTransferExtrinsic(
        mojom::kPolkadotTestnet, polkadot_testnet_account_->account_id.Clone(),
        std::variant<uint128_t, TransferAll>(uint128_t{1234}), recipient_pubkey,
        test_future.GetCallback());
    return test_future.Take();
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
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                        mojom::kPolkadotTestnetAssetHub);
  network_manager_->RemoveHiddenNetwork(mojom::CoinType::DOT,
                                        mojom::kPolkadotPaseoAssetHub);

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
    ASSERT_EQ(networks->size(), 3u);
    EXPECT_EQ((*networks)[0]->chain_id, mojom::kPolkadotTestnet);
    EXPECT_EQ((*networks)[0]->coin, mojom::CoinType::DOT);
    EXPECT_EQ((*networks)[1]->chain_id, mojom::kPolkadotTestnetAssetHub);
    EXPECT_EQ((*networks)[1]->coin, mojom::CoinType::DOT);
    EXPECT_EQ((*networks)[2]->chain_id, mojom::kPolkadotPaseoAssetHub);
    EXPECT_EQ((*networks)[2]->coin, mojom::CoinType::DOT);
  }
}

TEST_F(PolkadotWalletServiceUnitTest, GetAddress) {
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  polkadot_mock_rpc->FinalizeSetup();

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
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  polkadot_mock_rpc->FinalizeSetup();

  // Invalid chain_id should fail before metadata fetch.
  {
    base::test::TestFuture<mojom::PolkadotValidationStatus> future;
    polkadot_wallet_service->ValidateAddressForTransaction(
        "unknown-chain-id", "158HHeYTmEXMiMM1XufQt5bEe2CTia3EcVcfrpYBYcXA6bdb",
        future.GetCallback());
    EXPECT_EQ(future.Get(),
              mojom::PolkadotValidationStatus::kInvalidAddressFormat);
  }

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
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  polkadot_mock_rpc->UseInvalidChainMetadata();
  polkadot_mock_rpc->FinalizeSetup();

  base::test::TestFuture<mojom::PolkadotValidationStatus> future;
  polkadot_wallet_service->ValidateAddressForTransaction(
      mojom::kPolkadotMainnet,
      "158HHeYTmEXMiMM1XufQt5bEe2CTia3EcVcfrpYBYcXA6bdb", future.GetCallback());
  EXPECT_EQ(future.Get(),
            mojom::PolkadotValidationStatus::kFailedToFetchMetadata);
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic) {
  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());

  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  UnlockWallet();

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> recipient_pubkey = {};
  EXPECT_TRUE(base::HexStringToSpan(kBob, recipient_pubkey));

  auto sender_pubkey =
      keyring_service_->GetPolkadotPubKey(polkadot_testnet_account_->account_id)
          .value();
  EXPECT_EQ(base::HexEncodeLower(sender_pubkey),
            "14bccfbad15c6327408e833d162271f93a51fa3a6bc67d3eacc384bb9704d71e");

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->AddReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

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
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.UseInvalidChainMetadata(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoAccountInfo) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectAccountInfoRequest(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoChainHeader) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectInitialChainHeader(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoParentHeader) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectParentBlockHeader(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoFinalizedHead) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectFinalizedHead(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest,
       SignTransferExtrinsic_NoFinalizedBlockHeader) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectFinalizedBlockHeader(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoGenesisHash) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectGenesisBlockHash(); });
  EXPECT_EQ(signed_extrinsic.error(), WalletInternalErrorMessage());
}

TEST_F(PolkadotWalletServiceUnitTest, SignTransferExtrinsic_NoRuntimeVersion) {
  auto signed_extrinsic = SignTransferExtrinsicWithFailedStep(
      [](PolkadotMockRpc& rpc) { rpc.RejectRuntimeVersion(); });
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

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  static constexpr char kExpectedExtrinsic[] =
      "4d0284000e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
      "881b7e71e01821f988e7ca41ef24164bb2a1b948fcd9a2e97cd08f58ac5a898"
      "4cabfc83596670890d4df3dd15878b7282369aaa996261a05429e7f172c41c3d"
      "8c753981b88cb502040000000a0300ae70948d0c015b6c2b1ac46b8931ad630"
      "1f2c648f3f0adf71d08a68fe745561e0b00409452a303";

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->SetExpectedExtrinsic(kExpectedExtrinsic);
  polkadot_mock_rpc->SetSubmittedExtrinsicHash(
      "0xcc92467cebaee29feb3a8eba5ec1314f903cd69e6b0ab93832900737251f42db");
  polkadot_mock_rpc->AddWestendAssetHubReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

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

TEST_F(PolkadotWalletServiceUnitTest, SignAndSendTransaction_PaseoAssetHub) {
  // Captured transaction:
  // https://assethub-paseo.subscan.io/extrinsic/0xec9e1043a7dd8f045c86f6058d356193dd654c068126647a89ac5a92696fa5bb
  keyring_service_->Reset();
  GetAccountUtils().CreateWallet(kAssetHubMnemonic, kTestWalletPassword);

  auto sender =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 0);
  auto recipient =
      GetAccountUtils().EnsureAccount(mojom::KeyringId::kPolkadotTestnet, 1);
  ASSERT_TRUE(sender);
  ASSERT_TRUE(recipient);
  SetPolkadotMockRndSeed(mojom::KeyringId::kPolkadotTestnet, 1234);

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

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  static constexpr char kExpectedExtrinsic[] =
      "490284000e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b881b7e71e"
      "0136eef01ae13c4a7e7edf0d051e5da1a3619a70a14d32af4b4efa12624800bd48443c02"
      "fe3b15c8193510e75abd3034ffdf266f803eb4952cd35cf5a4972b9c85a502080000000a"
      "0300ae70948d0c015b6c2b1ac46b8931ad6301f2c648f3f0adf71d08a68fe745561e0700"
      "c12a9b64";

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->SetExpectedExtrinsic(kExpectedExtrinsic);
  polkadot_mock_rpc->SetSubmittedExtrinsicHash(
      "0xec9e1043a7dd8f045c86f6058d356193dd654c068126647a89ac5a92696fa5bb");
  polkadot_mock_rpc->AddPaseoAssetHubReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

  base::test::TestFuture<base::expected<
      std::pair<std::string, PolkadotExtrinsicMetadata>, std::string>>
      future;

  polkadot_wallet_service->SignAndSendTransaction(
      mojom::kPolkadotPaseoAssetHub, sender->account_id.Clone(),
      std::variant<uint128_t, TransferAll>(uint128_t{432100000000ull}),
      recipient_pubkey, future.GetCallback());

  auto tx_hash = future.Take();
  ASSERT_TRUE(tx_hash.has_value());
  EXPECT_EQ(
      tx_hash->first,
      "0xec9e1043a7dd8f045c86f6058d356193dd654c068126647a89ac5a92696fa5bb");
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.extrinsic()),
            kExpectedExtrinsic);
  EXPECT_EQ(base::HexEncodeLower(tx_hash->second.block_hash()),
            "c3da0f76ab484260860d32dab28fb96f6b9a01b7c378587bebe37da88bb7f268");
  EXPECT_EQ(tx_hash->second.block_num(), 9683370u);
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

  auto polkadot_mock_rpc = std::make_unique<PolkadotMockRpc>(
      &url_loader_factory_, network_manager_.get());
  auto polkadot_wallet_service = std::make_unique<PolkadotWalletService>(
      *keyring_service_, *network_manager_, prefs_,
      url_loader_factory_.GetSafeWeakWrapper());

  static constexpr char kExpectedExtrinsic[] =
      "490284000e161e17289c260a07020cc2a23192e882d5bee006b1390deed844b"
      "881b7e71e019ab2d49ce64d7dd15a6b0beab4dbd71b29f5b661c2b98867e473"
      "18c7402d770133cd398d8ca7a1238d2a8e50142ac5d705a267e7eeab82ae146a"
      "4045b96256845501040000000a0300ae70948d0c015b6c2b1ac46b8931ad630"
      "1f2c648f3f0adf71d08a68fe745561e0700e40b5402";

  polkadot_mock_rpc->SetSenderPubKey(sender_pubkey);
  polkadot_mock_rpc->SetExpectedExtrinsic(kExpectedExtrinsic);
  polkadot_mock_rpc->SetSubmittedExtrinsicHash(
      "0x32cf17114363bc9b26256104cab96d19a1052b4407452e05709fb6876d64daff");
  polkadot_mock_rpc->AddPolkadotAssetHubReqResPairs();
  polkadot_mock_rpc->FinalizeSetup();

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
