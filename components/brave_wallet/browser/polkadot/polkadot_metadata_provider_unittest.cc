/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_metadata_provider.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata_prefs.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

using testing::_;
using testing::AtLeast;
using testing::NiceMock;
using testing::Property;
using testing::Return;

class MockPolkadotChainMetadataPrefs : public PolkadotChainMetadataPrefs {
 public:
  explicit MockPolkadotChainMetadataPrefs(PrefService& profile_prefs)
      : PolkadotChainMetadataPrefs(profile_prefs) {}

  MOCK_METHOD(std::optional<PolkadotChainMetadata>,
              GetChainMetadata,
              (std::string_view chain_id),
              (const, override));
  MOCK_METHOD(bool,
              SetChainMetadata,
              (std::string_view chain_id, const PolkadotChainMetadata& metadata),
              (override));
};

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
              (std::string_view chain_id, OptionalBlockHash block_hash,
               GetRuntimeVersionCallback callback),
              (override));
  MOCK_METHOD(void,
              GetMetadata,
              (std::string_view chain_id, GetMetadataCallback callback),
              (override));
};

std::string ReadMetadataHexFixture(std::string_view file_name) {
  std::string json_response;
  const auto fixture_path = base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
                                .AppendASCII("brave")
                                .AppendASCII("components")
                                .AppendASCII("brave_wallet")
                                .AppendASCII("browser")
                                .AppendASCII("polkadot")
                                .AppendASCII(file_name);
  CHECK(base::ReadFileToString(fixture_path, &json_response));

  auto json = base::JSONReader::ReadDict(json_response, 0);
  CHECK(json.has_value());

  const std::string* metadata_hex = json->FindString("result");
  CHECK(metadata_hex);
  return *metadata_hex;
}

class PolkadotMetadataProviderUnitTest : public testing::Test {
 public:
  PolkadotMetadataProviderUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

 protected:
  void SetUp() override {
    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    mock_prefs_ =
        std::make_unique<NiceMock<MockPolkadotChainMetadataPrefs>>(profile_prefs_);
    mock_rpc_ = std::make_unique<NiceMock<MockPolkadotSubstrateRpc>>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());
    provider_ = std::make_unique<PolkadotMetadataProvider>(*mock_prefs_, *mock_rpc_);
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<NiceMock<MockPolkadotChainMetadataPrefs>> mock_prefs_;
  std::unique_ptr<NiceMock<MockPolkadotSubstrateRpc>> mock_rpc_;
  std::unique_ptr<PolkadotMetadataProvider> provider_;
};

// Verifies that Init() triggers warmup metadata fetches for both supported
// Polkadot networks.
TEST_F(PolkadotMetadataProviderUnitTest, InitInitialization) {
  base::test::TestFuture<void> mainnet_metadata_requested;
  base::test::TestFuture<void> testnet_metadata_requested;
  auto mainnet_requested_cb = mainnet_metadata_requested.GetCallback();
  auto testnet_requested_cb = testnet_metadata_requested.GetCallback();

  EXPECT_CALL(*mock_prefs_, GetChainMetadata(_))
      .Times(2)
      .WillRepeatedly(Return(std::nullopt));
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
        std::move(cb).Run(std::nullopt, std::make_optional<std::string>("rpc_error"));
      });

  provider_->Init();
  EXPECT_TRUE(mainnet_metadata_requested.Wait());
  EXPECT_TRUE(testnet_metadata_requested.Wait());
}

// Verifies that Init() schedules and performs retry when initial metadata fetch
// fails, and succeeds on subsequent attempt.
TEST_F(PolkadotMetadataProviderUnitTest, InitRetryCase) {
  const std::string metadata_hex =
      ReadMetadataHexFixture("state_getMetadata_polkadot.json");

  base::test::TestFuture<void> first_mainnet_attempt;
  base::test::TestFuture<void> second_mainnet_attempt;
  auto first_mainnet_attempt_cb = first_mainnet_attempt.GetCallback();
  auto second_mainnet_attempt_cb = second_mainnet_attempt.GetCallback();

  int mainnet_calls = 0;
  EXPECT_CALL(*mock_prefs_, GetChainMetadata(_))
      .Times(AtLeast(3))
      .WillRepeatedly(Return(std::nullopt));
  EXPECT_CALL(*mock_rpc_, GetMetadata(_, _))
      .Times(AtLeast(3))
      .WillRepeatedly([&](std::string_view chain_id,
                          PolkadotSubstrateRpc::GetMetadataCallback cb) {
        if (chain_id == mojom::kPolkadotMainnet) {
          ++mainnet_calls;
          if (mainnet_calls == 1) {
            if (first_mainnet_attempt_cb) {
              std::move(first_mainnet_attempt_cb).Run();
            }
            std::move(cb).Run(std::nullopt, std::make_optional<std::string>("first_fail"));
            return;
          }
          if (mainnet_calls == 2 && second_mainnet_attempt_cb) {
            std::move(second_mainnet_attempt_cb).Run();
          }
        }
        std::move(cb).Run(std::make_optional(metadata_hex), std::nullopt);
      });
  EXPECT_CALL(*mock_prefs_, SetChainMetadata(_, _))
      .Times(AtLeast(2))
      .WillRepeatedly(Return(true));

  provider_->Init();
  EXPECT_TRUE(first_mainnet_attempt.Wait());
  EXPECT_EQ(mainnet_calls, 1);

  task_environment_.FastForwardBy(base::Seconds(5));
  EXPECT_TRUE(second_mainnet_attempt.Wait());
  EXPECT_GE(mainnet_calls, 2);
}

// Verifies that when persisted metadata is present and runtime spec version
// matches, data is served from cache without network metadata refresh.
TEST_F(PolkadotMetadataProviderUnitTest, DataIsRestoredFromCache) {
  auto saved_metadata = PolkadotChainMetadata::FromFields(5, 1, 0, 100);
  ASSERT_TRUE(saved_metadata);

  EXPECT_CALL(*mock_prefs_, GetChainMetadata(mojom::kPolkadotMainnet))
      .WillOnce(Return(saved_metadata));
  EXPECT_CALL(*mock_rpc_, GetRuntimeVersion(mojom::kPolkadotMainnet, _, _))
      .WillOnce([](std::string_view, MockPolkadotSubstrateRpc::OptionalBlockHash,
                   PolkadotSubstrateRpc::GetRuntimeVersionCallback cb) {
        PolkadotRuntimeVersion version;
        version.spec_version = 100;
        std::move(cb).Run(std::make_optional(version), std::nullopt);
      });
  EXPECT_CALL(*mock_rpc_, GetMetadata(_, _)).Times(0);
  EXPECT_CALL(*mock_prefs_, SetChainMetadata(_, _)).Times(0);

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>> future;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future.GetCallback());
  auto result = future.Take();

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->GetSpecVersion(), 100u);
  EXPECT_EQ(result->GetBalancesPalletIndex(), 5u);
}

// Verifies that on cache miss metadata is fetched from network, persisted to
// prefs, and then served from in-memory cache on subsequent request.
TEST_F(PolkadotMetadataProviderUnitTest, DataIsSavedToCache) {
  const std::string metadata_hex =
      ReadMetadataHexFixture("state_getMetadata_polkadot.json");

  EXPECT_CALL(*mock_prefs_, GetChainMetadata(mojom::kPolkadotMainnet))
      .Times(1)
      .WillOnce(Return(std::nullopt));
  EXPECT_CALL(*mock_rpc_, GetMetadata(mojom::kPolkadotMainnet, _))
      .Times(1)
      .WillOnce([&](std::string_view, PolkadotSubstrateRpc::GetMetadataCallback cb) {
        std::move(cb).Run(std::make_optional(metadata_hex), std::nullopt);
      });
  EXPECT_CALL(*mock_prefs_, SetChainMetadata(mojom::kPolkadotMainnet, _))
      .Times(1)
      .WillOnce(Return(true));

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>> future1;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future1.GetCallback());
  auto result1 = future1.Take();
  ASSERT_TRUE(result1.has_value());

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>> future2;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future2.GetCallback());
  auto result2 = future2.Take();
  ASSERT_TRUE(result2.has_value());
}

// Verifies that when persisted metadata spec version is stale, provider fetches
// fresh metadata and updates stored cache entry.
TEST_F(PolkadotMetadataProviderUnitTest, CachedDataIsUpdated) {
  const std::string metadata_hex =
      ReadMetadataHexFixture("state_getMetadata_polkadot.json");
  auto saved_metadata = PolkadotChainMetadata::FromFields(1, 1, 0, 1);
  ASSERT_TRUE(saved_metadata);

  EXPECT_CALL(*mock_prefs_, GetChainMetadata(mojom::kPolkadotMainnet))
      .WillOnce(Return(saved_metadata));
  EXPECT_CALL(*mock_rpc_, GetRuntimeVersion(mojom::kPolkadotMainnet, _, _))
      .WillOnce([](std::string_view, MockPolkadotSubstrateRpc::OptionalBlockHash,
                   PolkadotSubstrateRpc::GetRuntimeVersionCallback cb) {
        PolkadotRuntimeVersion version;
        version.spec_version = 2;
        std::move(cb).Run(std::make_optional(version), std::nullopt);
      });
      
  EXPECT_CALL(*mock_rpc_, GetMetadata(mojom::kPolkadotMainnet, _))
      .WillOnce([&](std::string_view, PolkadotSubstrateRpc::GetMetadataCallback cb) {
        std::move(cb).Run(std::make_optional(metadata_hex), std::nullopt);
      });
  EXPECT_CALL(
      *mock_prefs_,
      SetChainMetadata(
          mojom::kPolkadotMainnet,
          Property(&PolkadotChainMetadata::GetSpecVersion, testing::Ne(1u))))
      .WillOnce(Return(true));

  base::test::TestFuture<base::expected<PolkadotChainMetadata, std::string>> future;
  provider_->GetChainMetadata(mojom::kPolkadotMainnet, future.GetCallback());
  auto result = future.Take();

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->GetSpecVersion(), 1022000u);
  EXPECT_EQ(result->GetBalancesPalletIndex(), 4u);
  EXPECT_EQ(result->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(result->GetSs58Prefix(), 42u);
}

}  // namespace
}  // namespace brave_wallet
