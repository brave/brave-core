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
      /*ss58_prefix=*/42, /*spec_version=*/1'234'567);
  EXPECT_EQ(metadata.GetSystemPalletIndex(), 0u);
  EXPECT_EQ(metadata.GetBalancesPalletIndex(), 7u);
  EXPECT_EQ(metadata.GetTransactionPaymentPalletIndex(), 0x20u);
  EXPECT_EQ(metadata.GetTransferAllowDeathCallIndex(), 2u);
  EXPECT_EQ(metadata.GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata.GetSpecVersion(), 1'234'567u);
}

TEST(PolkadotChainMetadataUnitTest, FromChainName) {
  auto westend = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(westend);
  EXPECT_EQ(westend->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(westend->GetBalancesPalletIndex(), 4u);
  EXPECT_EQ(westend->GetTransactionPaymentPalletIndex(), 0x1au);
  EXPECT_EQ(westend->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(westend->GetSs58Prefix(), 42u);

  auto westend_asset_hub =
      PolkadotChainMetadata::FromChainName("Westend Asset Hub");
  ASSERT_TRUE(westend_asset_hub);
  EXPECT_EQ(westend_asset_hub->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(westend_asset_hub->GetBalancesPalletIndex(), 10u);
  EXPECT_EQ(westend_asset_hub->GetTransactionPaymentPalletIndex(), 0x0bu);
  EXPECT_EQ(westend_asset_hub->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(westend_asset_hub->GetSs58Prefix(), 42u);

  auto polkadot = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(polkadot);
  EXPECT_EQ(polkadot->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(polkadot->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(polkadot->GetTransactionPaymentPalletIndex(), 0x20u);
  EXPECT_EQ(polkadot->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(polkadot->GetSs58Prefix(), 0u);

  auto polkadot_asset_hub =
      PolkadotChainMetadata::FromChainName("Polkadot Asset Hub");
  ASSERT_TRUE(polkadot_asset_hub);
  EXPECT_EQ(polkadot_asset_hub->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(polkadot_asset_hub->GetBalancesPalletIndex(), 10u);
  EXPECT_EQ(polkadot_asset_hub->GetTransactionPaymentPalletIndex(), 0x0bu);
  EXPECT_EQ(polkadot_asset_hub->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(polkadot_asset_hub->GetSs58Prefix(), 0u);

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
  EXPECT_EQ(metadata->GetSs58Prefix(), 0u);
  EXPECT_EQ(metadata->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(metadata->GetSpecVersion(), 2'000'007u);
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
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 4u);
  EXPECT_EQ(metadata->GetSpecVersion(), 1022000u);
}

// ---------------------------------------------------------------------------
// Security validation tests: these tests document parsing bugs that could
// lead to incorrect extrinsic construction (wrong pallet/call indices) and
// thus potential loss of funds.  Each test is labelled with a concern number
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
// The parser always uses the v15 Vec format regardless of the version byte,
// so v14 metadata with Map storage entries where the hasher byte is != 0
// will be misinterpreted.  When hasher=0 (Blake2_128), the byte 0x00
// accidentally decodes as Compact(0) → empty Vec, so it works by luck.
// When hasher=1 (Blake2_256), byte 0x01 triggers two-byte Compact mode,
// overconsuming the next byte and causing a parse failure or silent misparse.
//
// Impact: A malicious or legacy v14 RPC node could return metadata that the
// parser silently misparses, producing wrong pallet/call indices that flow
// directly into extrinsic encoding for fund transfers.

// v14 metadata with no storage entries — baseline that should parse correctly.
TEST(PolkadotChainMetadataUnitTest, Security_V14NoStorage_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d00000008285353353850726566697808"
      "082a00001c56657273696f6e0428000000000000640000000000002042616c616e636573"
      "00010000000005485472616e73616374696f6e5061796d656e74000000000020",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(metadata->GetSpecVersion(), 100u);
}

// v14 metadata with Map storage, hasher=0 (Blake2_128).
// Byte 0x00 accidentally decodes as Compact(0)=empty Vec, so this works.
TEST(PolkadotChainMetadataUnitTest, Security_V14MapStorageHasher0_ParsesByLuck) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "740101000808040000000008285353353850726566697808082a00001c56657273696f6e04"
      "28000000000000640000000000002042616c616e63657300010000000005485472616e7361"
      "6374696f6e5061796d656e74000000000020",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// v14 metadata with Map storage, hasher=1 (Blake2_256).
// Byte 0x01 triggers two-byte Compact mode, consuming the next byte and
// producing a bogus vec length. The parser then fails to decode.
// BUG: The parser accepts version=14 but doesn't handle v14 Map format.
TEST(PolkadotChainMetadataUnitTest, Security_V14MapStorageHasher1_FailsToParse) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610e0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "740101010808040000000008285353353850726566697808082a00001c56657273696f6e04"
      "28000000000000640000000000002042616c616e63657300010000000005485472616e7361"
      "6374696f6e5061796d656e74000000000020",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  // BUG: This should parse correctly for v14 metadata. Instead it returns
  // nullopt because the parser always uses the v15 Vec<StorageHasher> format
  // for Map storage entries, which is incompatible with v14's single-byte
  // hasher encoding.
  EXPECT_FALSE(metadata);
}

// v15 metadata with the same Map storage entry (hasher=1 as Vec<u8>).
// This is the correct format and should parse without issues.
TEST(PolkadotChainMetadataUnitTest, Security_V15MapStorageHasher1_ParsesCorrectly) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "74010104010808040000000008285353353850726566697808082a00001c56657273696f6e"
      "042800000000000064000000000000002042616c616e6365730001000000000500485472"
      "616e73616374696f6e5061796d656e7400000000002000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// Concern #2: decode_ss58_prefix doesn't handle Compact<u32> encoding.
//
// The SS58Prefix constant is SCALE-encoded, but its concrete integer width
// varies across runtimes (u8, u16, u32, or Compact<u32>). The parser tries
// fixed-width u16, u32, u8 in order, checking that all bytes are consumed.
//
// When a runtime encodes SS58Prefix as Compact<u32>(42), the bytes are
// [0xa8] (single-byte Compact: (42 << 2) | 0b00 = 168). The parser's u16
// decode fails (needs 2 bytes), u32 decode fails (needs 4 bytes), then u8
// decode succeeds reading 0xa8=168. The parser returns ss58_prefix=168
// instead of the correct value 42.
//
// Impact: Wrong ss58_prefix flows into ParsePolkadotAccount, which decodes
// recipient addresses. A wrong prefix could cause address misinterpretation,
// potentially sending funds to a different address than intended.

// Control: v15 metadata with SS58Prefix as u16(42)=[0x2a, 0x00] → returns 42.
TEST(PolkadotChainMetadataUnitTest, Security_SS58PrefixU16_ReturnsCorrectValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "74010104010808040000000008285353353850726566697808082a00001c56657273696f6e"
      "042800000000000064000000000000002042616c616e6365730001000000000500485472"
      "616e73616374696f6e5061796d656e7400000000002000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
}

// BUG: v15 metadata with SS58Prefix as Compact<u32>(42)=[0xa8].
// The parser reads 0xa8 as u8=168, returning ss58_prefix=168 instead of 42.
TEST(PolkadotChainMetadataUnitTest, Security_SS58PrefixCompact_ReturnsWrongValue) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "7401010401080804000000000828535335385072656669780804a8001c56657273696f6e"
      "042800000000000064000000000000002042616c616e6365730001000000000500485472"
      "616e73616374696f6e5061796d656e7400000000002000",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  ASSERT_TRUE(metadata);
  // BUG: ss58_prefix should be 42 but the parser returns 168 because
  // decode_ss58_prefix doesn't attempt Compact<u32> decoding before the
  // fixed-width fallbacks.  Compact<u32>(42) encodes as [0xa8], which the
  // u8 fallback reads as the raw byte value 168.
  EXPECT_EQ(metadata->GetSs58Prefix(), 168u);
}

// Concern #3: Parser does not validate that all input bytes are consumed.
//
// After reading all expected fields, the parser never checks that the input
// buffer is empty. If the metadata format is subtly different from what the
// parser expects (e.g., an extra field in a future version), the parser could
// silently succeed with wrong values while leaving trailing data unread.
//
// Impact: A compromised RPC could append arbitrary bytes to a valid metadata
// response. While the current parser reads specific fields in order and would
// likely still extract correct values, the lack of a trailing-bytes check
// means format drift would go undetected.

TEST(PolkadotChainMetadataUnitTest,
     Security_TrailingBytesNotRejected_ShouldFail) {
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(
      "0x6d6574610f0c0000000104507472616e736665725f616c6c6f775f6465617468000000"
      "0004043852756e74696d6556657273696f6e0000000008041c4163636f756e7404045401"
      "08000800000000000000000000c1853797374656d011853797374656d041c4163636f756e"
      "74010104010808040000000008285353353850726566697808082a00001c56657273696f6e"
      "042800000000000064000000000000002042616c616e6365730001000000000500485472"
      "616e73616374696f6e5061796d656e7400000000002000deadbeef",
      &bytes));
  auto metadata = PolkadotChainMetadata::FromBytes(bytes);
  // BUG: FromBytes should reject metadata with trailing bytes, but it
  // silently accepts them. The metadata values happen to be correct here
  // because the parser reads fields in order and stops after the last one
  // it needs, ignoring the trailing 0xdeadbeef.
  ASSERT_TRUE(metadata);
  EXPECT_EQ(metadata->GetSs58Prefix(), 42u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
}

// Concern #4: Unbounded Vec::with_capacity on untrusted vec length.
//
// decode_vec reads a Compact<u32> length from the input and immediately
// calls Vec::with_capacity(len) without any bounds check. A malicious RPC
// response with a huge Compact length could cause the browser to allocate
// excessive memory before the first element decode fails.
//
// This is a DoS vector — no fund loss, but it could crash the browser tab.
// The test below uses a moderate length that won't OOM but demonstrates the
// parser doesn't validate lengths before allocation.
//
// Note: A truly malicious length (e.g., Compact(u32::MAX)) could allocate
// several GB before failing. The fix would be to add a reasonable upper
// bound in decode_vec_len (e.g., reject lengths > 10_000).

TEST(PolkadotChainMetadataUnitTest,
     Security_HugeVecLength_ParserDoesNotValidateLength) {
  // Valid magic + version 15, then a Compact<u32>(50000) for the portable
  // registry types vec. There is nowhere near enough data for 50000 types,
  // so the parser will fail — but it allocates a HashMap and iterates before
  // detecting the insufficient data.
  std::vector<uint8_t> bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes("0x6d6574610f420d0300", &bytes));
  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(bytes));
}

}  // namespace brave_wallet
