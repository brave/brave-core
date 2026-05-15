/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_assets_metadata_provider.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

using testing::_;
using testing::NiceMock;

class MockPolkadotSubstrateRpc : public PolkadotSubstrateRpc {
 public:
  MockPolkadotSubstrateRpc(
      NetworkManager& network_manager,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
      : PolkadotSubstrateRpc(network_manager, std::move(url_loader_factory)) {}

  MOCK_METHOD(void,
              GetSupportedAssets,
              (std::string_view chain_id, GetSupportedAssetsCallback callback),
              (override));
  MOCK_METHOD(void,
              GetAssetMetadata,
              (std::string_view chain_id,
               uint32_t asset_id,
               GetAssetMetadataCallback callback),
              (override));
  MOCK_METHOD(void,
              GetStorageHash,
              (std::string_view chain_id,
               std::string_view storage_path,
               GetStorageHashCallback callback),
              (override));
};

class PolkadotChainAssetsMetadataProviderUnitTest : public testing::Test {
 protected:
  void SetUp() override {
    RegisterProfilePrefs(profile_prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    chain_assets_pref_ =
        std::make_unique<PolkadotChainAssetsPref>(profile_prefs_);
    mock_rpc_ = std::make_unique<NiceMock<MockPolkadotSubstrateRpc>>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());
    provider_ = std::make_unique<PolkadotChainAssetsMetadataProvider>(
        *chain_assets_pref_, *mock_rpc_);
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<PolkadotChainAssetsPref> chain_assets_pref_;
  std::unique_ptr<NiceMock<MockPolkadotSubstrateRpc>> mock_rpc_;
  std::unique_ptr<PolkadotChainAssetsMetadataProvider> provider_;
};

std::array<uint8_t, kPolkadotBlockHashSize> MakeHash(uint8_t seed) {
  std::array<uint8_t, kPolkadotBlockHashSize> hash = {};
  hash[0] = seed;
  return hash;
}

std::vector<uint8_t> MakeAssetMetadataBytes() {
  return {
      1,  0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // deposit
      20, 'T', 'o', 'k', 'e', 'n',                                // name
      12, 'T', 'O', 'K',                                          // symbol
      12,                                                         // decimals
      0,                                                          // is_frozen
  };
}

TEST_F(PolkadotChainAssetsMetadataProviderUnitTest,
       ResolveMetadataFetchesAndCaches) {
  EXPECT_CALL(*mock_rpc_, GetStorageHash(mojom::kPolkadotMainnet, _, _))
      .Times(2)
      .WillOnce([](std::string_view, std::string_view,
                   PolkadotSubstrateRpc::GetStorageHashCallback callback) {
        std::move(callback).Run(base::ok(std::make_optional(MakeHash(1))));
      })
      .WillOnce([](std::string_view, std::string_view,
                   PolkadotSubstrateRpc::GetStorageHashCallback callback) {
        std::move(callback).Run(base::ok(std::make_optional(MakeHash(2))));
      });
  EXPECT_CALL(*mock_rpc_, GetSupportedAssets(mojom::kPolkadotMainnet, _))
      .WillOnce([](std::string_view,
                   PolkadotSubstrateRpc::GetSupportedAssetsCallback callback) {
        std::move(callback).Run(base::ok(std::vector<uint32_t>{1, 1984}));
      });
  EXPECT_CALL(*mock_rpc_, GetAssetMetadata(mojom::kPolkadotMainnet, 1984, _))
      .WillOnce([](std::string_view, uint32_t,
                   PolkadotSubstrateRpc::GetAssetMetadataCallback callback) {
        std::move(callback).Run(
            base::ok(std::make_optional(MakeAssetMetadataBytes())));
      });

  base::test::TestFuture<
      base::expected<std::optional<AssetMetadata>, std::string>>
      future;
  provider_->ResolveMetadata(mojom::kPolkadotMainnet, 1984,
                             future.GetCallback());
  auto result = future.Take();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->has_value());
  EXPECT_EQ((*result)->symbol(), (std::vector<uint8_t>{'T', 'O', 'K'}));

  EXPECT_TRUE(
      chain_assets_pref_->GetAssetMetadata(mojom::kPolkadotMainnet, 1984));
}

TEST_F(PolkadotChainAssetsMetadataProviderUnitTest,
       ResolveMetadataUsesCachedMetadataWhenHashesMatch) {
  ASSERT_TRUE(chain_assets_pref_->SetAssetStorageHash(
      mojom::kPolkadotMainnet, std::make_optional(MakeHash(1))));
  ASSERT_TRUE(chain_assets_pref_->SetMetadataStorageHash(
      mojom::kPolkadotMainnet, std::make_optional(MakeHash(2))));
  const std::vector<uint32_t> supported_assets = {1984};
  ASSERT_TRUE(chain_assets_pref_->SetSupportedAssets(mojom::kPolkadotMainnet,
                                                     supported_assets));
  auto metadata = AssetMetadata::FromBytes(MakeAssetMetadataBytes());
  ASSERT_TRUE(metadata);
  ASSERT_TRUE(chain_assets_pref_->SetAssetMetadata(mojom::kPolkadotMainnet,
                                                   1984, *metadata));

  EXPECT_CALL(*mock_rpc_, GetStorageHash(mojom::kPolkadotMainnet, _, _))
      .Times(2)
      .WillOnce([](std::string_view, std::string_view,
                   PolkadotSubstrateRpc::GetStorageHashCallback callback) {
        std::move(callback).Run(base::ok(std::make_optional(MakeHash(1))));
      })
      .WillOnce([](std::string_view, std::string_view,
                   PolkadotSubstrateRpc::GetStorageHashCallback callback) {
        std::move(callback).Run(base::ok(std::make_optional(MakeHash(2))));
      });
  EXPECT_CALL(*mock_rpc_, GetSupportedAssets(_, _)).Times(0);
  EXPECT_CALL(*mock_rpc_, GetAssetMetadata(_, _, _)).Times(0);

  base::test::TestFuture<
      base::expected<std::optional<AssetMetadata>, std::string>>
      future;
  provider_->ResolveMetadata(mojom::kPolkadotMainnet, 1984,
                             future.GetCallback());
  auto result = future.Take();

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->has_value());
  EXPECT_EQ(**result, *metadata);
}

}  // namespace
}  // namespace brave_wallet
