/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_assets.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {
namespace {

constexpr char kAssetOneStorageKey[] =
    "0x682a59d51ab9e48a8c8cc418ff9708d2d34371a193a751eea5883e9553457b2e"
    "d82c12285b5d4551f88e8f6e7eb52b8101000000";
constexpr char kAsset1984StorageKey[] =
    "0x682a59d51ab9e48a8c8cc418ff9708d2d34371a193a751eea5883e9553457b2e"
    "a319d0e87221ca1ee751c1529f201522c0070000";

std::vector<uint8_t> MakeAssetMetadataBytes() {
  std::vector<uint8_t> bytes = {
      1,  0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // deposit
      20, 'T', 'o', 'k', 'e', 'n',                                // name
      12, 'T', 'O', 'K',                                          // symbol
      12,                                                         // decimals
      1,                                                          // is_frozen
  };
  return bytes;
}

TEST(PolkadotAssetsUnitTest, ParsesAssetsList) {
  const std::vector<std::string> storage_keys = {kAssetOneStorageKey,
                                                 kAsset1984StorageKey};

  auto assets_list = AssetsList::FromStorageKeys(storage_keys);
  ASSERT_TRUE(assets_list);
  EXPECT_EQ(assets_list->identifiers(), (std::vector<uint32_t>{1, 1984}));
}

TEST(PolkadotAssetsUnitTest, RejectsInvalidAssetsListKey) {
  const std::vector<std::string> storage_keys = {
      "0x0000000000000000000000000000000000000000000000000000000000000000"
      "d82c12285b5d4551f88e8f6e7eb52b8101000000"};

  EXPECT_FALSE(AssetsList::FromStorageKeys(storage_keys));
}

TEST(PolkadotAssetsUnitTest, ParsesAssetMetadata) {
  auto metadata = AssetMetadata::FromBytes(MakeAssetMetadataBytes());
  ASSERT_TRUE(metadata);

  std::array<uint8_t, 16> expected_deposit = {1};
  EXPECT_TRUE(std::ranges::equal(metadata->deposit(), expected_deposit));
  EXPECT_EQ(metadata->name(), (std::vector<uint8_t>{'T', 'o', 'k', 'e', 'n'}));
  EXPECT_EQ(metadata->symbol(), (std::vector<uint8_t>{'T', 'O', 'K'}));
  EXPECT_EQ(metadata->decimals(), 12);
  EXPECT_TRUE(metadata->is_frozen());
}

TEST(PolkadotAssetsUnitTest, RejectsTrailingAssetMetadataBytes) {
  auto bytes = MakeAssetMetadataBytes();
  bytes.push_back(0);

  EXPECT_FALSE(AssetMetadata::FromBytes(bytes));
}

}  // namespace
}  // namespace brave_wallet
