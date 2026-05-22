/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_chain_metadata.h"

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

}  // namespace

TEST(PolkadotChainMetadataUnitTest, EqualityOperator) {
  PolkadotChainMetadata metadata_a;
  metadata_a->system_pallet_index = 0;
  metadata_a->balances_pallet_index = 7;
  metadata_a->transaction_payment_pallet_index = 0x20;
  metadata_a->transfer_allow_death_call_index = 2;
  metadata_a->transfer_keep_alive_call_index = 4;
  metadata_a->transfer_all_call_index = 5;
  metadata_a->ss58_prefix = 42;
  metadata_a->spec_version = 1'234'567;
  metadata_a->asset_tx_payment = false;

  PolkadotChainMetadata metadata_b = metadata_a;

  PolkadotChainMetadata metadata_c = metadata_a;
  metadata_c->spec_version = 1'234'568;

  PolkadotChainMetadata metadata_d = metadata_a;
  metadata_d->asset_tx_payment = true;

  EXPECT_EQ(metadata_a, metadata_b);
  EXPECT_NE(metadata_a, metadata_c);
  EXPECT_NE(metadata_a, metadata_d);
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
  PolkadotChainMetadata expected;
  auto& [system_pallet_index, balances_pallet_index,
         transaction_payment_pallet_index, transfer_allow_death_call_index,
         transfer_keep_alive_call_index, transfer_all_call_index, ss58_prefix,
         spec_version, asset_tx_payment] = *expected;
  system_pallet_index = 0;
  balances_pallet_index = 5;
  transaction_payment_pallet_index = 0x20;
  transfer_allow_death_call_index = 0;
  transfer_keep_alive_call_index = 3;
  transfer_all_call_index = 4;
  ss58_prefix = 0;
  spec_version = 2'000'007;
  asset_tx_payment = false;
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
  PolkadotChainMetadata expected;
  auto& [system_pallet_index, balances_pallet_index,
         transaction_payment_pallet_index, transfer_allow_death_call_index,
         transfer_keep_alive_call_index, transfer_all_call_index, ss58_prefix,
         spec_version, asset_tx_payment] = *expected;
  system_pallet_index = 0;
  balances_pallet_index = 0x0a;
  transaction_payment_pallet_index = 0x0b;
  transfer_allow_death_call_index = 0;
  transfer_keep_alive_call_index = 3;
  transfer_all_call_index = 4;
  ss58_prefix = 0;
  spec_version = 2'002'001;
  asset_tx_payment = true;
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
  PolkadotChainMetadata expected;
  auto& [system_pallet_index, balances_pallet_index,
         transaction_payment_pallet_index, transfer_allow_death_call_index,
         transfer_keep_alive_call_index, transfer_all_call_index, ss58_prefix,
         spec_version, asset_tx_payment] = *expected;
  system_pallet_index = 0;
  balances_pallet_index = 4;
  transaction_payment_pallet_index = 0x1a;
  transfer_allow_death_call_index = 0;
  transfer_keep_alive_call_index = 3;
  transfer_all_call_index = 4;
  ss58_prefix = 42;
  spec_version = 1'022'000;
  asset_tx_payment = false;
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

  PolkadotChainMetadata expected;
  auto& [system_pallet_index, balances_pallet_index,
         transaction_payment_pallet_index, transfer_allow_death_call_index,
         transfer_keep_alive_call_index, transfer_all_call_index, ss58_prefix,
         spec_version, asset_tx_payment] = *expected;
  system_pallet_index = 0;
  balances_pallet_index = 0x0a;
  transaction_payment_pallet_index = 0x0b;
  transfer_allow_death_call_index = 0;
  transfer_keep_alive_call_index = 3;
  transfer_all_call_index = 4;
  ss58_prefix = 42;
  spec_version = 1'022'005;
  asset_tx_payment = true;
  EXPECT_EQ(*metadata, expected);

  auto metadata2 = PolkadotMetadataFromChainName("Westend Asset Hub");
  ASSERT_TRUE(metadata2);
  PolkadotChainMetadata expected_from_name = expected;
  expected_from_name->spec_version = 0;
  EXPECT_EQ(*metadata2, expected_from_name);
}

}  // namespace brave_wallet
