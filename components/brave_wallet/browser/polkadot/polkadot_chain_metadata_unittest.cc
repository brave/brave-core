/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include <vector>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kResult[] = "result";

}  // namespace

TEST(PolkadotChainMetadataUnitTest, FromFields) {
  auto metadata = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567);

  EXPECT_EQ(metadata.GetSystemPalletIndex(), 0u);
  EXPECT_EQ(metadata.GetBalancesPalletIndex(), 7u);
  EXPECT_EQ(metadata.GetTransactionPaymentPalletIndex(), 0x20u);
  EXPECT_EQ(metadata.GetTransferAllowDeathCallIndex(), 2u);
  EXPECT_EQ(metadata.GetTransferKeepAliveCallIndex(), 4u);
  EXPECT_EQ(metadata.GetTransferAllCallIndex(), 5u);
  EXPECT_EQ(metadata.GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata.GetSpecVersion(), 1'234'567u);
}

TEST(PolkadotChainMetadataUnitTest, EqualityOperator) {
  auto metadata_a = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567);
  auto metadata_b = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567);
  auto metadata_c = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/7,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/2,
      /*transfer_keep_alive_call_index=*/4,
      /*transfer_all_call_index=*/5,
      /*ss58_prefix=*/42, /*spec_version=*/1'234'568);

  EXPECT_EQ(metadata_a, metadata_b);
  EXPECT_NE(metadata_a, metadata_c);
}

TEST(PolkadotChainMetadataUnitTest, FromChainName) {
  auto expected_westend = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0);
  auto westend = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(westend);

  EXPECT_EQ(*westend, expected_westend);

  auto expected_westend_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0);
  auto westend_asset_hub =
      PolkadotChainMetadata::FromChainName("Westend Asset Hub");
  ASSERT_TRUE(westend_asset_hub);
  EXPECT_EQ(*westend_asset_hub, expected_westend_asset_hub);

  auto expected_polkadot = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0);
  auto polkadot = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(polkadot);
  EXPECT_EQ(*polkadot, expected_polkadot);

  auto expected_polkadot_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0);
  auto polkadot_asset_hub =
      PolkadotChainMetadata::FromChainName("Polkadot Asset Hub");
  ASSERT_TRUE(polkadot_asset_hub);
  EXPECT_EQ(*polkadot_asset_hub, expected_polkadot_asset_hub);

  EXPECT_FALSE(PolkadotChainMetadata::FromChainName(""));
  EXPECT_FALSE(PolkadotChainMetadata::FromChainName("Unknown Chain"));
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

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponsePolkadot) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://rpc.polkadot.io/
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_polkadot.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/2'000'007);
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(metadata2);
  auto expected_from_name = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/5,
      /*transaction_payment_pallet_index=*/0x20,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0);
  EXPECT_EQ(*metadata2, expected_from_name);
}

TEST(PolkadotChainMetadataUnitTest, ParseRealStateGetMetadataResponseWestend) {
  // Refreshed with:
  // curl -sS -H 'Content-Type: application/json' \
  //   -d '{"id":1,"jsonrpc":"2.0","method":"state_getMetadata","params":[]}' \
  //   https://westend-rpc.polkadot.io/
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_westend.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

  auto metadata = PolkadotChainMetadata::FromBytes(metadata_bytes);
  ASSERT_TRUE(metadata);
  auto expected = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/1'022'000);
  EXPECT_EQ(*metadata, expected);
}

// ---------------------------------------------------------------------------
// Security validation tests: each test is labelled with a concern number
// that maps to the security review findings for PR #34784.
// ---------------------------------------------------------------------------

// Concern #1: v14 Map storage format mismatch.
//
// In RuntimeMetadataV14, StorageEntryType::Map encodes the hasher as a
// single StorageHasher enum byte:
//   Map { hasher: StorageHasher, key: TypeId, value: TypeId }
//
// In RuntimeMetadataV15, it was changed to a Vec:
//   Map { hashers: Vec<StorageHasher>, key: TypeId, value: TypeId }
//
// FIX: The parser now tries Vec<StorageHasher> first, and falls back to a
// single byte on decode failure, handling both v14 and v15 formats.

// v14 metadata with no storage entries — baseline that should parse correctly.
TEST(PolkadotChainMetadataUnitTest, Security_V14NoStorage_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "000000640000000000002042616c616e63657300010400000005485472616e73"
      "616374696f6e5061796d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(metadata->GetSpecVersion(), 100u);
}

// v14 metadata with Map storage, hasher=0 (Blake2_128).
// Vec<StorageHasher> decode succeeds: 0x00 decodes as Compact(0)=empty vec.
TEST(PolkadotChainMetadataUnitTest, Security_V14MapStorageHasher0_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e7400010000000000000008285353353850726566697800082a00001c56"
      "657273696f6e005820706f6c6b61646f74106e6f646501000000640000000000"
      "002042616c616e63657300010400000005485472616e73616374696f6e506179"
      "6d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// v14 metadata with Map storage, hasher=1 (Blake2_256).
// Vec<StorageHasher> decode fails (0x01 triggers two-byte Compact mode),
// but the single-byte fallback succeeds.
TEST(PolkadotChainMetadataUnitTest, Security_V14MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e7400010100000000000008285353353850726566697800082a00001c56"
      "657273696f6e005820706f6c6b61646f74106e6f646501000000640000000000"
      "002042616c616e63657300010400000005485472616e73616374696f6e506179"
      "6d656e74000000000003",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// v15 metadata with Map storage, hasher=1 encoded as Vec<StorageHasher>([1]).
TEST(PolkadotChainMetadataUnitTest, Security_V15MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d011853797374656d041c416363"
      "6f756e740001040100000000000008285353353850726566697800082a00001c"
      "56657273696f6e005820706f6c6b61646f74106e6f6465010000006400000000"
      "0000002042616c616e6365730001040000000500485472616e73616374696f6e"
      "5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// Concern #2: decode_ss58_prefix doesn't handle Compact<u32> encoding.
//
// The SS58Prefix constant is SCALE-encoded, but its concrete integer width
// varies across runtimes (u8, u16, u32, or Compact<u32>).
//
// FIX: The parser now tries u16, u32, Compact<u32>, then u8 in order,
// checking that all bytes are consumed. Compact<u32> is tried after u16/u32
// to avoid misinterpreting multi-byte fixed-width encodings as compact.

// v15 metadata with SS58Prefix as u16(42)=[0x2a, 0x00] → returns 42.
TEST(PolkadotChainMetadataUnitTest, Security_SS58PrefixU16_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "00000064000000000000002042616c616e636573000104000000050048547261"
      "6e73616374696f6e5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
}

// v15 metadata with SS58Prefix as Compact<u32>(42)=[0xa8] → returns 42.
TEST(PolkadotChainMetadataUnitTest, Security_SS58PrefixCompact_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "69780004a8001c56657273696f6e005820706f6c6b61646f74106e6f64650100"
      "000064000000000000002042616c616e6365730001040000000500485472616e"
      "73616374696f6e5061796d656e7400000000000300",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
}

// Concern #3: Parser does not validate that all input bytes are consumed.
//
// After reading all expected fields, the parser never checks that the input
// buffer is empty. A compromised RPC could append arbitrary bytes to valid
// metadata. Note: real metadata also has trailing fields (extrinsics, APIs,
// outer_event) that this partial parser doesn't consume, so a trailing-bytes
// check cannot be added without breaking real-world parsing.
//
// This test documents the known limitation.

TEST(PolkadotChainMetadataUnitTest,
     Security_TrailingBytesNotRejected_KnownLimitation) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f080000000000000400000104507472616e736665725f616c6c6f"
      "775f6465617468000000000c1853797374656d00000008285353353850726566"
      "697800082a00001c56657273696f6e005820706f6c6b61646f74106e6f646501"
      "00000064000000000000002042616c616e636573000104000000050048547261"
      "6e73616374696f6e5061796d656e7400000000000300deadbeef",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  // Known limitation: trailing bytes are silently accepted. Cannot add a
  // trailing-bytes check because real metadata has fields after the pallets
  // section that this partial parser doesn't consume.
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// Concern #4: Unbounded Vec::with_capacity on untrusted vec length.
//
// FIX: decode_vec_len now rejects vec lengths > 10_000 before allocation,
// preventing a DoS via maliciously large Compact length values.

TEST(PolkadotChainMetadataUnitTest,
     Security_HugeVecLength_RejectedByBoundsCheck) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x6d6574610f420d0300", &bytes));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(bytes));
}

}  // namespace brave_wallet
