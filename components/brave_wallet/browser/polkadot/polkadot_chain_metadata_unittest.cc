/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::string ScaleEncodeString(std::string_view value) {
  auto bytes = base::as_byte_span(value);
  auto encoded = scale_encode_string(::rust::Slice(bytes));
  return std::string(encoded.begin(), encoded.end());
}

}  // namespace

TEST(PolkadotChainMetadataUnitTest, FromFields) {
  auto metadata = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/true,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/10,
      /*assets_transfer_keep_alive_call_index=*/9);

  EXPECT_EQ(metadata->system_pallet_index, 0u);
  EXPECT_EQ(metadata->balances_pallet_index, 7u);
  EXPECT_EQ(metadata->transaction_payment_pallet_index, 0x20u);
  EXPECT_EQ(metadata->transfer_allow_death_call_index, 2u);
  EXPECT_EQ(metadata->transfer_keep_alive_call_index, 4u);
  EXPECT_EQ(metadata->transfer_all_call_index, 5u);
  EXPECT_TRUE(metadata->has_assets_pallet);
  EXPECT_EQ(metadata->assets_pallet_index, 50u);
  EXPECT_EQ(metadata->assets_transfer_all_call_index, 10u);
  EXPECT_EQ(metadata->assets_transfer_keep_alive_call_index, 9u);
  EXPECT_EQ(metadata->ss58_prefix, 42u);
  EXPECT_EQ(metadata->spec_version, 1'234'567u);
  EXPECT_TRUE(metadata->asset_tx_payment);
}

TEST(PolkadotChainMetadataUnitTest, EqualityOperator) {
  auto metadata_a = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);

  PolkadotChainMetadata metadata_b = metadata_a;

  PolkadotChainMetadata metadata_c = metadata_a;
  metadata_c->spec_version = 1'234'568;

  PolkadotChainMetadata metadata_d = metadata_a;
  metadata_d->asset_tx_payment = true;

  auto metadata_e = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/10,
      /*assets_transfer_keep_alive_call_index=*/9);

  EXPECT_EQ(metadata_a, metadata_b);
  EXPECT_NE(metadata_a, metadata_c);
  EXPECT_NE(metadata_a, metadata_d);
  EXPECT_NE(metadata_a, metadata_e);
}

TEST(PolkadotChainMetadataUnitTest, FromBytesInvalid) {
  // Invalid metadata magic/version payload.
  std::vector<uint8_t> invalid_magic;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x6d65746164617461", &invalid_magic));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(invalid_magic));

  std::vector<uint8_t> invalid_short;
  ASSERT_TRUE(PrefixedHexStringToBytes("0xdeadbeef", &invalid_short));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(invalid_short));
}

TEST(PolkadotChainMetadataUnitTest,
     ParseMetadataWithoutAssetsPalletLeavesAssetsFieldsUnset) {
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_polkadot.json");
  ASSERT_FALSE(metadata_bytes.empty());

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  EXPECT_FALSE((*metadata)->has_assets_pallet);
  EXPECT_EQ((*metadata)->assets_pallet_index, 0u);
  EXPECT_EQ((*metadata)->assets_transfer_all_call_index, 0u);
  EXPECT_EQ((*metadata)->assets_transfer_keep_alive_call_index, 0u);
}

TEST(PolkadotChainMetadataUnitTest,
     ParseMetadataWithAssetsPalletMissingMethodFails) {
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_assethub_polkadot.json");
  ASSERT_FALSE(metadata_bytes.empty());

  // The second transfer_keep_alive occurrence is the Assets pallet call
  // variant; the first belongs to Balances.
  const std::string transfer_keep_alive =
      ScaleEncodeString("transfer_keep_alive");
  const std::string transfer_dead_alive =
      ScaleEncodeString("transfer_dead_alive");
  ASSERT_TRUE(ReplaceNthOccurrence(metadata_bytes, transfer_keep_alive,
                                   transfer_dead_alive, /*occurrence=*/1));

  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(metadata_bytes));
}

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponsePolkadot) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://rpc.polkadot.io/
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_polkadot.json");

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/2'000'007,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotMetadataFromChainName("Polkadot");
  ASSERT_TRUE(metadata2);
  PolkadotChainMetadata expected_from_name = expected;
  expected_from_name->spec_version = 0;
  EXPECT_EQ(*metadata2, expected_from_name);
}

TEST(PolkadotChainMetadataUnitTest,
     ParseRealStateGetMetadataResponseAssetHubPolkadot) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://polkadot-asset-hub-rpc.polkadot.io
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_assethub_polkadot.json");

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/0x0a,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/2'002'001,
      /*asset_tx_payment=*/true,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/32,
      /*assets_transfer_keep_alive_call_index=*/9);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotMetadataFromChainName("Polkadot Asset Hub");
  ASSERT_TRUE(metadata2);
  PolkadotChainMetadata expected_from_name = expected;
  expected_from_name->spec_version = 0;
  EXPECT_EQ(*metadata2, expected_from_name);
}

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponseWestend) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://westend-rpc.polkadot.io/
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_westend.json");

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/1'022'000,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotMetadataFromChainName("Westend");
  ASSERT_TRUE(metadata2);
  PolkadotChainMetadata expected_from_name = expected;
  expected_from_name->spec_version = 0;
  EXPECT_EQ(*metadata2, expected_from_name);
}

TEST(PolkadotChainMetadataUnitTest,
     ParseRealStateGetMetadataResponseAssetHubWestend) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://westend-asset-hub-rpc.polkadot.io/
  std::vector<uint8_t> metadata_bytes =
      ReadMetadataFixture("state_getMetadata_assethub_westend.json");

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);

  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/0x0a,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/1'022'005,
      /*asset_tx_payment=*/true,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/32,
      /*assets_transfer_keep_alive_call_index=*/9);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotMetadataFromChainName("Westend Asset Hub");
  ASSERT_TRUE(metadata2);
  PolkadotChainMetadata expected_from_name = expected;
  expected_from_name->spec_version = 0;
  EXPECT_EQ(*metadata2, expected_from_name);
}

}  // namespace brave_wallet
