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

TEST(PolkadotChainMetadataUnitTest, Security_V14NoStorage_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V14, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610e08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet has no storage; constants encode SS58Prefix=42 and
      // Version.spec_version=100.
      "0c1853797374656d00000008285353353850726566697800082a00001c56657273696f6e"
      "005820706f6c6b61646f74106e6f64650100000064000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e63657300010400000005485472616e73616374696f6e5061796d656e7400"
      "0000000003"
      // Minimal V14 extrinsic metadata with no signed extensions.
      "000400",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V14MapStorageHasher0_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V14, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610e08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet includes Account map storage using StorageHasher
      // discriminant 0; constants encode SS58Prefix=42 and spec_version=100.
      "0c1853797374656d011853797374656d041c4163636f756e740001000000000000000828"
      "5353353850726566697800082a00001c56657273696f6e005820706f6c6b61646f74106e"
      "6f64650100000064000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e63657300010400000005485472616e73616374696f6e5061796d656e7400"
      "0000000003"
      // Minimal V14 extrinsic metadata with no signed extensions.
      "000400",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V14MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V14, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610e08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet includes Account map storage using StorageHasher
      // discriminant 1; constants encode SS58Prefix=42 and spec_version=100.
      "0c1853797374656d011853797374656d041c4163636f756e740001010000000000000828"
      "5353353850726566697800082a00001c56657273696f6e005820706f6c6b61646f74106e"
      "6f64650100000064000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e63657300010400000005485472616e73616374696f6e5061796d656e7400"
      "0000000003"
      // Minimal V14 extrinsic metadata with no signed extensions.
      "000400",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_V15MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V15, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610f08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet includes Account map storage with one hasher
      // (discriminant 1), empty pallet docs, SS58Prefix=42, and
      // spec_version=100.
      "0c1853797374656d011853797374656d041c4163636f756e740001040100000000000008"
      "285353353850726566697800082a00001c56657273696f6e005820706f6c6b61646f7410"
      "6e6f6465010000006400000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e6365730001040000000500485472616e73616374696f6e5061796d656e74"
      "00000000000300"
      // Minimal V15 extrinsic metadata with no signed extensions.
      "040000000000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_SS58PrefixU16_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V15, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610f08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet has no storage; SS58Prefix is encoded as fixed-width
      // u16(42), with Version.spec_version=100.
      "0c1853797374656d00000008285353353850726566697800082a00001c56657273696f6e"
      "005820706f6c6b61646f74106e6f6465010000006400000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e6365730001040000000500485472616e73616374696f6e5061796d656e74"
      "00000000000300"
      // Minimal V15 extrinsic metadata with no signed extensions.
      "040000000000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_SS58PrefixCompact_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V15, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610f08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet has no storage; SS58Prefix is encoded as
      // Compact<u32>(42), with Version.spec_version=100.
      "0c1853797374656d0000000828535335385072656669780004a8001c56657273696f6e00"
      "5820706f6c6b61646f74106e6f6465010000006400000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e6365730001040000000500485472616e73616374696f6e5061796d656e74"
      "00000000000300"
      // Minimal V15 extrinsic metadata with no signed extensions.
      "040000000000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_TrailingBytesNotRejected_KnownLimitation) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V15, and portable
      // registry. Balances calls include transfer_allow_death(0),
      // transfer_keep_alive(3), and transfer_all(4).
      "0x6d6574610f08000000000000040000010c507472616e736665725f616c6c6f775f6465"
      "6174680000004c7472616e736665725f6b6565705f616c697665000300307472616e7366"
      "65725f616c6c00040000"
      // System pallet has no storage; constants encode SS58Prefix=42 and
      // Version.spec_version=100.
      "0c1853797374656d00000008285353353850726566697800082a00001c56657273696f6e"
      "005820706f6c6b61646f74106e6f6465010000006400000000000000"
      // Balances pallet index 5 references the call enum above;
      // TransactionPayment pallet index 3 is present for fee metadata.
      "2042616c616e6365730001040000000500485472616e73616374696f6e5061796d656e74"
      "00000000000300"
      // Minimal V15 extrinsic metadata with no signed extensions.
      "040000000000"
      // Extra bytes preserved for the trailing-bytes limitation check.
      "deadbeef",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  // Known limitation: trailing bytes are silently accepted. Cannot add a
  // trailing-bytes check because real metadata may include fields after the
  // subset this partial parser consumes.
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/3,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/100,
      /*asset_tx_payment=*/false,
      /*has_assets_pallet=*/false,
      /*assets_pallet_index=*/0,
      /*assets_transfer_all_call_index=*/0,
      /*assets_transfer_keep_alive_call_index=*/0);
  EXPECT_EQ(*metadata, expected);
}

TEST(PolkadotChainMetadataUnitTest,
     Security_HugeVecLength_RejectedByBoundsCheck) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      // RuntimeMetadataPrefixed: magic "meta", V15, then an oversized
      // PortableRegistry vector length rejected by bounds checks.
      "0x6d6574610f420d0300", &bytes));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(bytes));
}

}  // namespace brave_wallet
