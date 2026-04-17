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

TEST(PolkadotChainMetadataUnitTest, FromChainName) {
  auto westend = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(westend);
  EXPECT_EQ(westend->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(westend->GetBalancesPalletIndex(), 4u);
  EXPECT_EQ(westend->GetTransactionPaymentPalletIndex(), 0x1au);
  EXPECT_EQ(westend->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(westend->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(westend->GetTransferAllCallIndex(), 4u);
  EXPECT_EQ(westend->GetSs58Prefix(), 42u);

  auto westend_asset_hub =
      PolkadotChainMetadata::FromChainName("Westend Asset Hub");
  ASSERT_TRUE(westend_asset_hub);
  EXPECT_EQ(westend_asset_hub->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(westend_asset_hub->GetBalancesPalletIndex(), 10u);
  EXPECT_EQ(westend_asset_hub->GetTransactionPaymentPalletIndex(), 0x0bu);
  EXPECT_EQ(westend_asset_hub->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(westend_asset_hub->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(westend_asset_hub->GetTransferAllCallIndex(), 4u);
  EXPECT_EQ(westend_asset_hub->GetSs58Prefix(), 42u);

  auto polkadot = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(polkadot);
  EXPECT_EQ(polkadot->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(polkadot->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(polkadot->GetTransactionPaymentPalletIndex(), 0x20u);
  EXPECT_EQ(polkadot->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(polkadot->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(polkadot->GetTransferAllCallIndex(), 4u);
  EXPECT_EQ(polkadot->GetSs58Prefix(), 0u);

  auto polkadot_asset_hub =
      PolkadotChainMetadata::FromChainName("Polkadot Asset Hub");
  ASSERT_TRUE(polkadot_asset_hub);
  EXPECT_EQ(polkadot_asset_hub->GetSystemPalletIndex(), 0u);
  EXPECT_EQ(polkadot_asset_hub->GetBalancesPalletIndex(), 10u);
  EXPECT_EQ(polkadot_asset_hub->GetTransactionPaymentPalletIndex(), 0x0bu);
  EXPECT_EQ(polkadot_asset_hub->GetTransferAllowDeathCallIndex(), 0u);
  EXPECT_EQ(polkadot_asset_hub->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(polkadot_asset_hub->GetTransferAllCallIndex(), 4u);
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
  EXPECT_EQ(metadata->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(metadata->GetTransferAllCallIndex(), 4u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 5u);
  EXPECT_EQ(metadata->GetSpecVersion(), 2'000'007u);

  auto metadata2 = PolkadotChainMetadata::FromChainName("Polkadot");
  ASSERT_TRUE(metadata2);
  EXPECT_EQ(metadata2->GetSs58Prefix(), metadata->GetSs58Prefix());
  EXPECT_EQ(metadata2->GetTransferAllowDeathCallIndex(),
            metadata->GetTransferAllowDeathCallIndex());
  EXPECT_EQ(metadata2->GetTransferKeepAliveCallIndex(),
            metadata->GetTransferKeepAliveCallIndex());
  EXPECT_EQ(metadata2->GetTransferAllCallIndex(),
            metadata->GetTransferAllCallIndex());
  EXPECT_EQ(metadata2->GetBalancesPalletIndex(),
            metadata->GetBalancesPalletIndex());
  EXPECT_EQ(metadata2->GetSpecVersion(), 0u);
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
  EXPECT_EQ(metadata->GetTransferKeepAliveCallIndex(), 3u);
  EXPECT_EQ(metadata->GetTransferAllCallIndex(), 4u);
  EXPECT_EQ(metadata->GetBalancesPalletIndex(), 4u);
  EXPECT_EQ(metadata->GetSpecVersion(), 1022000u);

  auto metadata2 = PolkadotChainMetadata::FromChainName("Westend");
  ASSERT_TRUE(metadata2);
  EXPECT_EQ(metadata2->GetSs58Prefix(), metadata->GetSs58Prefix());
  EXPECT_EQ(metadata2->GetTransferAllowDeathCallIndex(),
            metadata->GetTransferAllowDeathCallIndex());
  EXPECT_EQ(metadata2->GetTransferKeepAliveCallIndex(),
            metadata->GetTransferKeepAliveCallIndex());
  EXPECT_EQ(metadata2->GetTransferAllCallIndex(),
            metadata->GetTransferAllCallIndex());
  EXPECT_EQ(metadata2->GetBalancesPalletIndex(),
            metadata->GetBalancesPalletIndex());
  EXPECT_EQ(metadata2->GetSpecVersion(), 0u);
}

}  // namespace brave_wallet
