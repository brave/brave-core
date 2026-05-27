/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_metadata_provider.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

using testing::_;
using testing::NiceMock;

class MockPolkadotSubstrateRpc : public PolkadotSubstrateRpc {
 public:
  using OptionalBlockHash =
      std::optional<base::span<uint8_t, kPolkadotBlockHashSize>>;

  MockPolkadotSubstrateRpc(
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : PolkadotSubstrateRpc(network_manager, std::move(url_loader_factory)) {}

  MOCK_METHOD(void,
              GetRuntimeVersion,
              (std::string_view chain_id,
               OptionalBlockHash block_hash,
               GetRuntimeVersionCallback callback),
              (override));
  MOCK_METHOD(void,
              GetMetadata,
              (std::string_view chain_id, GetMetadataCallback callback),
              (override));
};

class PolkadotMetadataProviderUnitTest : public testing::Test {
 public:
  PolkadotMetadataProviderUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    chain_metadata_prefs_ =
        std::make_unique<PolkadotChainMetadataPrefs>(profile_prefs_);
    mock_rpc_ = std::make_unique<NiceMock<MockPolkadotSubstrateRpc>>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());
    provider_ = std::make_unique<PolkadotMetadataProvider>(
        *network_manager_, *chain_metadata_prefs_, *mock_rpc_);
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<PolkadotChainMetadataPrefs> chain_metadata_prefs_;
  std::unique_ptr<NiceMock<MockPolkadotSubstrateRpc>> mock_rpc_;
  std::unique_ptr<PolkadotMetadataProvider> provider_;
};

// Verifies that Init() triggers warmup metadata fetches for both supported
// Polkadot networks.
TEST_F(PolkadotMetadataProviderUnitTest, InitInitialization) {
  const std::vector<uint8_t> mainnet_metadata_bytes =
      ReadMetadataFixture("state_getMetadata_polkadot.json");
  const std::vector<uint8_t> testnet_metadata_bytes =
      ReadMetadataFixture("state_getMetadata_westend.json");

  base::test::TestFuture<void> mainnet_metadata_requested;
  base::test::TestFuture<void> testnet_metadata_requested;
  auto mainnet_requested_cb = mainnet_metadata_requested.GetCallback();
  auto testnet_requested_cb = testnet_metadata_requested.GetCallback();

  EXPECT_CALL(*mock_rpc_, GetMetadata(_, _))
      .Times(2)
      .WillRepeatedly([&](std::string_view chain_id,
                          PolkadotSubstrateRpc::GetMetadataCallback cb) {
        if (chain_id == mojom::kPolkadotMainnet && mainnet_requested_cb) {
          std::move(mainnet_requested_cb).Run();
        }
        if (chain_id == mojom::kPolkadotTestnet && testnet_requested_cb) {
          std::move(testnet_requested_cb).Run();
        }
        if (chain_id == mojom::kPolkadotMainnet) {
          std::move(cb).Run(base::ok(mainnet_metadata_bytes));
          return;
        }
        std::move(cb).Run(base::ok(testnet_metadata_bytes));
      });

  provider_->Init();
  EXPECT_TRUE(mainnet_metadata_requested.Wait());
  EXPECT_TRUE(testnet_metadata_requested.Wait());

  PolkadotChainMetadata expected_mainnet = MakePolkadotMetadata();
  expected_mainnet->spec_version = 2'000'007;

  auto mainnet_metadata =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(mainnet_metadata.has_value());
  EXPECT_EQ(mainnet_metadata, expected_mainnet);

  PolkadotChainMetadata expected_testnet = MakeWestendMetadata();
  expected_testnet->spec_version = 1'022'000;

  auto testnet_metadata =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotTestnet);
  ASSERT_TRUE(testnet_metadata.has_value());
  EXPECT_EQ(testnet_metadata, expected_testnet);
  EXPECT_NE(testnet_metadata, mainnet_metadata);
}

// Verifies that Init() schedules and performs retry when initial metadata fetch
// fails, and succeeds on subsequent attempt.
TEST_F(PolkadotMetadataProviderUnitTest, InitRetryCase) {
  const std::vector<uint8_t> mainnet_metadata_bytes =
      ReadMetadataFixture("state_getMetadata_polkadot.json");
  const std::vector<uint8_t> testnet_metadata_bytes =
      ReadMetadataFixture("state_getMetadata_westend.json");

  base::test::TestFuture<void> first_mainnet_attempt;
  base::test::TestFuture<void> second_mainnet_attempt;
  auto first_mainnet_attempt_cb = first_mainnet_attempt.GetCallback();
  auto second_mainnet_attempt_cb = second_mainnet_attempt.GetCallback();

  int mainnet_calls = 0;
  EXPECT_CALL(*mock_rpc_, GetMetadata(_, _))
      .Times(3)
      .WillRepeatedly([&](std::string_view chain_id,
                          PolkadotSubstrateRpc::GetMetadataCallback cb) {
        if (chain_id == mojom::kPolkadotMainnet) {
          ++mainnet_calls;
          if (mainnet_calls == 1) {
            if (first_mainnet_attempt_cb) {
              std::move(first_mainnet_attempt_cb).Run();
            }
            std::move(cb).Run(base::unexpected<std::string>("first_fail"));
            return;
          }
          if (mainnet_calls == 2 && second_mainnet_attempt_cb) {
            std::move(second_mainnet_attempt_cb).Run();
          }
          std::move(cb).Run(base::ok(mainnet_metadata_bytes));
          return;
        }
        std::move(cb).Run(base::ok(testnet_metadata_bytes));
      });

  provider_->Init();
  EXPECT_TRUE(first_mainnet_attempt.Wait());
  EXPECT_EQ(mainnet_calls, 1);

  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(second_mainnet_attempt.Wait());
  EXPECT_GE(mainnet_calls, 2);

  PolkadotChainMetadata expected_mainnet = MakePolkadotMetadata();
  expected_mainnet->spec_version = 2'000'007;

  PolkadotChainMetadata expected_testnet = MakeWestendMetadata();
  expected_testnet->spec_version = 1'022'000;

  auto mainnet_metadata =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(mainnet_metadata.has_value());
  EXPECT_EQ(mainnet_metadata, expected_mainnet);

  auto testnet_metadata =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotTestnet);
  ASSERT_TRUE(testnet_metadata.has_value());
  EXPECT_EQ(testnet_metadata, expected_testnet);
}

// Verifies that when persisted metadata is present and runtime spec version
// matches, data is served from cache without network metadata refresh.
TEST_F(PolkadotMetadataProviderUnitTest, DataIsRestoredFromCache) {
  PolkadotChainMetadata saved_metadata = MakePolkadotMetadata();
  saved_metadata->spec_version = 100;
  ASSERT_TRUE(chain_metadata_prefs_->SetChainMetadata(mojom::kPolkadotMainnet,
                                                      saved_metadata));

  EXPECT_CALL(*mock_rpc_, GetRuntimeVersion(mojom::kPolkadotMainnet, _, _))
      .WillOnce([](std::string_view chain_id,
                   MockPolkadotSubstrateRpc::OptionalBlockHash,
                   PolkadotSubstrateRpc::GetRuntimeVersionCallback cb) {
        EXPECT_EQ(chain_id, mojom::kPolkadotMainnet);
        PolkadotRuntimeVersion version;
        version.spec_version = 100;
        std::move(cb).Run(std::make_optional(version), std::nullopt);
      });
  EXPECT_CALL(*mock_rpc_, GetMetadata(_, _)).Times(0);

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>>
      future;

  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, saved_metadata);

  auto stored =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored, saved_metadata);
}

// Verifies that on cache miss metadata is fetched from network, persisted to
// prefs, and then served from in-memory cache on subsequent request.
TEST_F(PolkadotMetadataProviderUnitTest, DataIsSavedToCache) {
  const std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_polkadot.json");

  EXPECT_CALL(*mock_rpc_, GetMetadata(mojom::kPolkadotMainnet, _))
      .Times(1)
      .WillOnce(
          [&](std::string_view, PolkadotSubstrateRpc::GetMetadataCallback cb) {
            std::move(cb).Run(base::ok(metadata_bytes));
          });

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>>
      future1;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future1.GetCallback());
  auto result1 = future1.Take();
  ASSERT_TRUE(result1.has_value());

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>>
      future2;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future2.GetCallback());
  auto result2 = future2.Take();
  ASSERT_TRUE(result2.has_value());

  EXPECT_EQ(*result1, *result2);

  auto stored =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored, *result1);
}

// Verifies that when persisted metadata spec version is stale, provider fetches
// fresh metadata and updates stored cache entry.
TEST_F(PolkadotMetadataProviderUnitTest, CachedDataIsUpdated) {
  const std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_westend.json");
  PolkadotChainMetadata saved_metadata = MakeWestendMetadata();
  saved_metadata->balances_pallet_index = 1;
  saved_metadata->transaction_payment_pallet_index = 32;
  saved_metadata->ss58_prefix = 0;
  saved_metadata->spec_version = 1;
  ASSERT_TRUE(chain_metadata_prefs_->SetChainMetadata(mojom::kPolkadotMainnet,
                                                      saved_metadata));

  EXPECT_CALL(*mock_rpc_, GetRuntimeVersion(mojom::kPolkadotMainnet, _, _))
      .WillOnce([](std::string_view,
                   MockPolkadotSubstrateRpc::OptionalBlockHash,
                   PolkadotSubstrateRpc::GetRuntimeVersionCallback cb) {
        PolkadotRuntimeVersion version;
        version.spec_version = 2;
        std::move(cb).Run(std::make_optional(version), std::nullopt);
      });

  EXPECT_CALL(*mock_rpc_, GetMetadata(mojom::kPolkadotMainnet, _))
      .WillOnce(
          [&](std::string_view, PolkadotSubstrateRpc::GetMetadataCallback cb) {
            std::move(cb).Run(base::ok(metadata_bytes));
          });

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>>
      future;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future.GetCallback());
  auto result = future.Take();

  ASSERT_TRUE(result.has_value());

  PolkadotChainMetadata expected = MakeWestendMetadata();
  expected->spec_version = 1'022'000;

  EXPECT_EQ(*result, expected);

  auto stored =
      chain_metadata_prefs_->GetChainMetadata(mojom::kPolkadotMainnet);
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(*stored, expected);
}

}  // namespace
}  // namespace brave_wallet
