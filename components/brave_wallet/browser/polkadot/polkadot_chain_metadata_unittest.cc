/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

#include <string_view>
#include <vector>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_test_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kResult[] = "result";

std::vector<uint8_t> ReadMetadataFixture(const char* fixture_name) {
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII(fixture_name);
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  EXPECT_TRUE(metadata_hex);
  if (!metadata_hex || metadata_hex->empty()) {
    return {};
  }

  std::vector<uint8_t> metadata_bytes;
  EXPECT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));
  return metadata_bytes;
}

bool ReplaceNthOccurrence(std::vector<uint8_t>* bytes,
                          std::string_view needle,
                          std::string_view replacement,
                          size_t occurrence) {
  if (needle.size() != replacement.size() || needle.empty()) {
    return false;
  }

  size_t matches = 0;
  for (size_t i = 0; i + needle.size() <= bytes->size(); ++i) {
    bool matched = true;
    for (size_t j = 0; j < needle.size(); ++j) {
      if ((*bytes)[i + j] != static_cast<uint8_t>(needle[j])) {
        matched = false;
        break;
      }
    }

    if (!matched) {
      continue;
    }

    if (matches == occurrence) {
      for (size_t j = 0; j < replacement.size(); ++j) {
        (*bytes)[i + j] = static_cast<uint8_t>(replacement[j]);
      }
      return true;
    }
    ++matches;
  }

  return false;
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
      /*asset_tx_payment=*/false);

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

TEST(PolkadotChainMetadataUnitTest, FromChainName) {
  auto expected_westend = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/4,
      /*transaction_payment_pallet_index=*/0x1a,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0,
      /*asset_tx_payment=*/false);
  auto westend = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(westend);

  EXPECT_EQ(*westend, expected_westend);

  auto expected_westend_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/42, /*spec_version=*/0,
      /*asset_tx_payment=*/true,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/32,
      /*assets_transfer_keep_alive_call_index=*/9);
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
      /*ss58_prefix=*/0, /*spec_version=*/0,
      /*asset_tx_payment=*/false);
  auto polkadot = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(polkadot);
  EXPECT_EQ(*polkadot, expected_polkadot);

  auto expected_polkadot_asset_hub = PolkadotChainMetadata::FromFields(
      /*system_pallet_index=*/0, /*balances_pallet_index=*/10,
      /*transaction_payment_pallet_index=*/0x0b,
      /*transfer_allow_death_call_index=*/0,
      /*transfer_keep_alive_call_index=*/3,
      /*transfer_all_call_index=*/4,
      /*ss58_prefix=*/0, /*spec_version=*/0,
      /*asset_tx_payment=*/true,
      /*has_assets_pallet=*/true,
      /*assets_pallet_index=*/50,
      /*assets_transfer_all_call_index=*/32,
      /*assets_transfer_keep_alive_call_index=*/9);
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

  // SCALE string length prefix 0x4c encodes 19 bytes. The second
  // transfer_keep_alive occurrence is the Assets pallet call variant; the first
  // belongs to Balances.
  constexpr std::string_view kTransferKeepAlive =
      "\x4c"
      "transfer_keep_alive";
  constexpr std::string_view kTransferDeadAlive =
      "\x4c"
      "transfer_dead_alive";
  ASSERT_TRUE(ReplaceNthOccurrence(&metadata_bytes, kTransferKeepAlive,
                                   kTransferDeadAlive, /*occurrence=*/1));

  EXPECT_FALSE(PolkadotChainMetadata::FromBytes(metadata_bytes));
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
      /*ss58_prefix=*/0, /*spec_version=*/2'000'007,
      /*asset_tx_payment=*/false);
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
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_assethub_polkadot.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

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
      /*ss58_prefix=*/42, /*spec_version=*/1'022'000,
      /*asset_tx_payment=*/false);
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
  const auto fixture_path =
      base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT)
          .AppendASCII("brave")
          .AppendASCII("components")
          .AppendASCII("test")
          .AppendASCII("data")
          .AppendASCII("brave_wallet")
          .AppendASCII("polkadot")
          .AppendASCII("chain_metadata")
          .AppendASCII("state_getMetadata_assethub_westend.json");
  const base::DictValue json = base::test::ParseJsonDictFromFile(fixture_path);

  const std::string* metadata_hex = json.FindString(kResult);
  ASSERT_TRUE(metadata_hex);
  ASSERT_FALSE(metadata_hex->empty());

  std::vector<uint8_t> metadata_bytes;
  ASSERT_TRUE(PrefixedHexStringToBytes(*metadata_hex, &metadata_bytes));

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
