/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_assets_pref.h"

#include <array>
#include <optional>
#include <vector>

#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

class PolkadotChainAssetsPrefUnitTest : public testing::Test {
 protected:
  void SetUp() override { RegisterProfilePrefs(profile_prefs_.registry()); }

  PolkadotChainAssetsPref MakePrefs() {
    return PolkadotChainAssetsPref(profile_prefs_);
  }

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
};

std::array<uint8_t, kPolkadotBlockHashSize> MakeHash(uint8_t seed) {
  std::array<uint8_t, kPolkadotBlockHashSize> hash = {};
  hash[0] = seed;
  return hash;
}

AssetMetadata MakeMetadata() {
  return AssetMetadata::FromFields(
      /*deposit=*/{1},
      /*name=*/std::vector<uint8_t>{'T', 'o', 'k', 'e', 'n'},
      /*symbol=*/std::vector<uint8_t>{'T', 'O', 'K'},
      /*decimals=*/12, /*is_frozen=*/true);
}

TEST_F(PolkadotChainAssetsPrefUnitTest, SetAndGetRoundTrip) {
  auto prefs = MakePrefs();
  ASSERT_TRUE(prefs.SetAssetStorageHash(mojom::kPolkadotMainnet,
                                        std::make_optional(MakeHash(1))));
  ASSERT_TRUE(prefs.SetMetadataStorageHash(mojom::kPolkadotMainnet,
                                           std::make_optional(MakeHash(2))));

  const std::vector<uint32_t> supported_assets = {1, 1984};
  ASSERT_TRUE(
      prefs.SetSupportedAssets(mojom::kPolkadotMainnet, supported_assets));

  auto metadata = MakeMetadata();
  ASSERT_TRUE(prefs.SetAssetMetadata(mojom::kPolkadotMainnet, 1984, metadata));

  EXPECT_EQ(prefs.GetAssetStorageHash(mojom::kPolkadotMainnet), MakeHash(1));
  EXPECT_EQ(prefs.GetMetadataStorageHash(mojom::kPolkadotMainnet), MakeHash(2));
  EXPECT_EQ(prefs.GetSupportedAssets(mojom::kPolkadotMainnet),
            supported_assets);

  auto loaded_metadata = prefs.GetAssetMetadata(mojom::kPolkadotMainnet, 1984);
  ASSERT_TRUE(loaded_metadata);
  EXPECT_EQ(*loaded_metadata, metadata);
}

TEST_F(PolkadotChainAssetsPrefUnitTest, ClearChainAssets) {
  auto prefs = MakePrefs();
  ASSERT_TRUE(prefs.SetAssetStorageHash(mojom::kPolkadotMainnet,
                                        std::make_optional(MakeHash(1))));
  prefs.ClearChainAssets(mojom::kPolkadotMainnet);

  EXPECT_FALSE(prefs.GetAssetStorageHash(mojom::kPolkadotMainnet));
  EXPECT_TRUE(prefs.GetSupportedAssets(mojom::kPolkadotMainnet).empty());
}

TEST_F(PolkadotChainAssetsPrefUnitTest, VersionMismatchClearsAssetsPrefs) {
  {
    ScopedDictPrefUpdate update(&profile_prefs_,
                                kBraveWalletPolkadotChainAssets);
    update->Set("version", PolkadotChainAssetsPref::kVersion + 1);
    update->Set(mojom::kPolkadotMainnet, base::DictValue());
  }

  auto prefs = MakePrefs();
  EXPECT_TRUE(profile_prefs_.GetDict(kBraveWalletPolkadotChainAssets).empty());
}

}  // namespace
}  // namespace brave_wallet
