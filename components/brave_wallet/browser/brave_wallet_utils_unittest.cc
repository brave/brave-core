/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/gtest_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using base::test::ParseJsonDict;
using testing::Contains;
using testing::ElementsAreArray;
using testing::Eq;
using testing::Not;

namespace brave_wallet {

TEST(BraveWalletUtilsUnitTest, EncodeString) {
  std::string output;
  EXPECT_TRUE(EncodeString("one", &output));
  EXPECT_EQ(output,
            // Count for input string.
            "0x0000000000000000000000000000000000000000000000000000000000000003"
            // Encoding for input string.
            "6f6e650000000000000000000000000000000000000000000000000000000000");

  output.clear();
  EXPECT_TRUE(EncodeString(
      "oneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneoneon"
      "e",
      &output));
  EXPECT_EQ(
      output,
      // Count for input string.
      "0x0000000000000000000000000000000000000000000000000000000000000048"
      // Encoding for input string.
      "6f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e"
      "656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f6e656f"
      "6e65000000000000000000000000000000000000000000000000");

  output.clear();
  EXPECT_TRUE(EncodeString("", &output));
  EXPECT_EQ(
      output,
      "0x0000000000000000000000000000000000000000000000000000000000000000");

  output.clear();
  std::string invalid_input = "\xF0\x8F\xBF\xBE";
  EXPECT_FALSE(base::IsStringUTF8(invalid_input));
  EXPECT_FALSE(EncodeString(invalid_input, &output));
}

TEST(BraveWalletUtilsUnitTest, EncodeStringArray) {
  std::vector<std::string> input({"one", "two", "three"});
  std::string output;
  EXPECT_TRUE(EncodeStringArray(input, &output));
  EXPECT_EQ(output,
            // count of elements in input array
            "0x0000000000000000000000000000000000000000000000000000000000000003"
            // offsets to array elements
            "0000000000000000000000000000000000000000000000000000000000000060"
            "00000000000000000000000000000000000000000000000000000000000000a0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"one", "one two three four five six seven eight nine", "two",
           "one two three four five six seven eight nine ten", "three"};
  output.clear();
  EXPECT_TRUE(EncodeStringArray(input, &output));

  EXPECT_EQ(output,
            // count of elements in input array
            "0x0000000000000000000000000000000000000000000000000000000000000005"
            // offsets to array elements
            "00000000000000000000000000000000000000000000000000000000000000a0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            "0000000000000000000000000000000000000000000000000000000000000140"
            "0000000000000000000000000000000000000000000000000000000000000180"
            "00000000000000000000000000000000000000000000000000000000000001e0"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for "one two three four five six seven eight nine"
            "000000000000000000000000000000000000000000000000000000000000002c"
            // encoding for "one two three four five six seven eight nine"
            "6f6e652074776f20746872656520666f75722066697665207369782073657665"
            "6e206569676874206e696e650000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for "one two three four five six seven eight nine ten"
            "0000000000000000000000000000000000000000000000000000000000000030"
            // encoding for "one two three four five six seven eight nine ten"
            "6f6e652074776f20746872656520666f75722066697665207369782073657665"
            "6e206569676874206e696e652074656e00000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"", "one", "", "two", "", "three"};
  output.clear();
  EXPECT_TRUE(EncodeStringArray(input, &output));

  EXPECT_EQ(output,
            "0x0000000000000000000000000000000000000000000000000000000000000006"
            // offsets to array elements
            "00000000000000000000000000000000000000000000000000000000000000c0"
            "00000000000000000000000000000000000000000000000000000000000000e0"
            "0000000000000000000000000000000000000000000000000000000000000120"
            "0000000000000000000000000000000000000000000000000000000000000140"
            "0000000000000000000000000000000000000000000000000000000000000180"
            "00000000000000000000000000000000000000000000000000000000000001a0"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "one"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "one"
            "6f6e650000000000000000000000000000000000000000000000000000000000"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "two"
            "0000000000000000000000000000000000000000000000000000000000000003"
            // encoding for "two"
            "74776f0000000000000000000000000000000000000000000000000000000000"
            // count for ""
            "0000000000000000000000000000000000000000000000000000000000000000"
            // count for "three"
            "0000000000000000000000000000000000000000000000000000000000000005"
            // encoding for "three"
            "7468726565000000000000000000000000000000000000000000000000000000");

  input = {"one", "\xF0\x8F\xBF\xBE"};
  output.clear();
  EXPECT_FALSE(EncodeStringArray(input, &output));
}

TEST(BraveWalletUtilsUnitTest, DecodeString) {
  std::string output;
  EXPECT_TRUE(DecodeString(
      0,
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // encoding for "one"
      "6f6e650000000000000000000000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "one");

  output.clear();
  EXPECT_TRUE(DecodeString(
      0,
      // count for "one two three four five six seven eight nine"
      "000000000000000000000000000000000000000000000000000000000000002c"
      // encoding for "one two three four five six seven eight nine"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e650000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "one two three four five six seven eight nine");

  output.clear();
  EXPECT_TRUE(DecodeString(
      0,
      // count for ""
      "0000000000000000000000000000000000000000000000000000000000000000",
      &output));
  EXPECT_EQ(output, "");

  // Test invalid inputs.
  output.clear();
  EXPECT_FALSE(DecodeString(0, "", &output));
  EXPECT_FALSE(DecodeString(0, "invalid string", &output));
  EXPECT_FALSE(DecodeString(
      0,
      // invalid count
      "6f6e650000000000000000000000000000000000000000000000000000000000",
      &output));

  EXPECT_FALSE(DecodeString(
      0,
      // count for "one"
      "0000000000000000000000000000000000000000000000000000000000000003"
      // invalid encoding for "one": len < expected len of encoding for "one"
      "6f6e",
      &output));

  EXPECT_FALSE(DecodeString(
      0,
      // count for "one" without encoding of string
      "0000000000000000000000000000000000000000000000000000000000000003",
      &output));

  EXPECT_FALSE(DecodeString(
      64,  // out-of-bound offset
      "0000000000000000000000000000000000000000000000000000000000000001",
      &output));

  EXPECT_FALSE(DecodeString(
      999999,  // out-of-bound invalid offset
      // count for "one two three four five six seven eight nine"
      "000000000000000000000000000000000000000000000000000000000000002c"
      // encoding for "one two three four five six seven eight nine"
      "6f6e652074776f20746872656520666f75722066697665207369782073657665"
      "6e206569676874206e696e650000000000000000000000000000000000000000",
      &output));
}

TEST(BraveWalletUtilsUnitTest, TransactionReceiptAndValue) {
  TransactionReceipt tx_receipt;
  tx_receipt.transaction_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  tx_receipt.transaction_index = 0x1;
  tx_receipt.block_number = 0xb;
  tx_receipt.block_hash =
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b";
  tx_receipt.cumulative_gas_used = 0x33bc;
  tx_receipt.gas_used = 0x4dc;
  tx_receipt.contract_address = "0xb60e8dd61c5d32be8058bb8eb970870f07233155";
  tx_receipt.status = true;

  base::Value::Dict tx_receipt_value = TransactionReceiptToValue(tx_receipt);
  auto tx_receipt_from_value = ValueToTransactionReceipt(tx_receipt_value);
  ASSERT_NE(tx_receipt_from_value, std::nullopt);
  EXPECT_EQ(tx_receipt, *tx_receipt_from_value);
}

TEST(BraveWalletUtilsTest, IsEndpointUsingBraveWalletProxy) {
  // Test with valid URLs that should match the proxy domains
  EXPECT_TRUE(IsEndpointUsingBraveWalletProxy(
      GURL("https://ethereum-mainnet.wallet.brave.com")));
  EXPECT_TRUE(IsEndpointUsingBraveWalletProxy(
      GURL("https://ethereum-mainnet.wallet.bravesoftware.com")));
  EXPECT_TRUE(IsEndpointUsingBraveWalletProxy(
      GURL("https://ethereum-mainnet.wallet.s.brave.io")));

  // Test with invalid URLs that should not match the proxy domains
  EXPECT_FALSE(IsEndpointUsingBraveWalletProxy(GURL("https://example.com")));
  EXPECT_FALSE(
      IsEndpointUsingBraveWalletProxy(GURL("https://wallet.brave.io")));
  EXPECT_FALSE(IsEndpointUsingBraveWalletProxy(GURL("https://brave.com")));
}

TEST(BraveWalletUtilsUnitTest, GetPrefKeyForCoinType) {
  auto key = GetPrefKeyForCoinType(mojom::CoinType::ETH);
  EXPECT_EQ(key, kEthereumPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::FIL);
  EXPECT_EQ(key, kFilecoinPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::SOL);
  EXPECT_EQ(key, kSolanaPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::BTC);
  EXPECT_EQ(key, kBitcoinPrefKey);
  key = GetPrefKeyForCoinType(mojom::CoinType::ZEC);
  EXPECT_EQ(key, kZCashPrefKey);

  EXPECT_TRUE(AllCoinsTested());

  EXPECT_NOTREACHED_DEATH(
      GetPrefKeyForCoinType(static_cast<mojom::CoinType>(2016)));
}

TEST(BraveWalletUtilsUnitTest, eTLDPlusOne) {
  EXPECT_EQ("", eTLDPlusOne(url::Origin()));
  EXPECT_EQ("brave.com",
            eTLDPlusOne(url::Origin::Create(GURL("https://blog.brave.com"))));
  EXPECT_EQ("brave.com",
            eTLDPlusOne(url::Origin::Create(GURL("https://...brave.com"))));
  EXPECT_EQ(
      "brave.com",
      eTLDPlusOne(url::Origin::Create(GURL("https://a.b.c.d.brave.com/1"))));
  EXPECT_EQ("brave.github.io", eTLDPlusOne(url::Origin::Create(GURL(
                                   "https://a.b.brave.github.io/example"))));
  EXPECT_EQ("", eTLDPlusOne(url::Origin::Create(GURL("https://github.io"))));
}

TEST(BraveWalletUtilsUnitTest, MakeOriginInfo) {
  auto origin_info =
      MakeOriginInfo(url::Origin::Create(GURL("https://blog.brave.com:443")));
  EXPECT_EQ("https://blog.brave.com", origin_info->origin_spec);
  EXPECT_EQ("brave.com", origin_info->e_tld_plus_one);

  url::Origin empty_origin;
  auto empty_origin_info = MakeOriginInfo(empty_origin);
  EXPECT_EQ("null", empty_origin_info->origin_spec);
  EXPECT_EQ("", empty_origin_info->e_tld_plus_one);
}

TEST(BraveWalletUtilsUnitTest, GenerateRandomHexString) {
  std::set<std::string> strings;
  for (int i = 0; i < 1000; ++i) {
    auto random_string = GenerateRandomHexString();
    EXPECT_EQ(64u, random_string.size());
    EXPECT_TRUE(base::ranges::all_of(random_string, base::IsHexDigit<char>));
    EXPECT_FALSE(base::Contains(strings, random_string));
    strings.insert(random_string);
  }
}

TEST(BraveWalletUtilsUnitTest, BitcoinNativeAssets) {
  EXPECT_EQ(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinMainnet)),
      ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "bitcoin_mainnet",
        "coin": 0,
        "coingecko_id": "btc",
        "decimals": 8,
        "is_compressed": false,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "spl_token_program": 1,
        "is_nft": false,
        "is_spam": false,
        "logo": "btc.png",
        "name": "Bitcoin",
        "symbol": "BTC",
        "token_id": "",
        "visible": true
      }
      )"));

  EXPECT_EQ(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinTestnet)),
      ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "bitcoin_testnet",
        "coin": 0,
        "coingecko_id": "",
        "decimals": 8,
        "is_compressed": false,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "spl_token_program": 1,
        "is_nft": false,
        "is_spam": false,
        "logo": "btc.png",
        "name": "Bitcoin",
        "symbol": "BTC",
        "token_id": "",
        "visible": true
      }
      )"));
}

TEST(BraveWalletUtilsUnitTest, ZcashNativeAssets) {
  EXPECT_EQ(BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashMainnet)),
            ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "zcash_mainnet",
        "coin": 133,
        "coingecko_id": "zec",
        "decimals": 8,
        "is_compressed": false,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "spl_token_program": 1,
        "is_nft": false,
        "is_spam": false,
        "logo": "zec.png",
        "name": "Zcash",
        "symbol": "ZEC",
        "token_id": "",
        "visible": true
      }
      )"));

  EXPECT_EQ(BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashTestnet)),
            ParseJsonDict(R"(
      {
        "address": "",
        "chain_id": "zcash_testnet",
        "coin": 133,
        "coingecko_id": "zec",
        "decimals": 8,
        "is_compressed": false,
        "is_erc1155": false,
        "is_erc20": false,
        "is_erc721": false,
        "spl_token_program": 1,
        "is_nft": false,
        "is_spam": false,
        "logo": "zec.png",
        "name": "Zcash",
        "symbol": "ZEC",
        "token_id": "",
        "visible": true
      }
      )"));
}

#if BUILDFLAG(ENABLE_ORCHARD)
TEST(BraveWalletUtilsUnitTest, DefaultZCashShieldedAssets_FeatureEnabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "true"}});

  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto assets = GetAllUserAssets(&prefs);

  {
    const auto count = base::ranges::count_if(
        assets, [](const mojom::BlockchainTokenPtr& item) {
          return item->is_shielded == true &&
                 item->coin == mojom::CoinType::ZEC;
        });

    EXPECT_EQ(2, count);
  }

  {
    const auto count = base::ranges::count_if(
        assets, [](const mojom::BlockchainTokenPtr& item) {
          return item->is_shielded == false &&
                 item->coin == mojom::CoinType::ZEC;
        });

    EXPECT_EQ(2, count);
  }
}

TEST(BraveWalletUtilsUnitTest, DefaultZCashShieldedAssets_FeatureDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeatureWithParameters(
      features::kBraveWalletZCashFeature,
      {{"zcash_shielded_transactions_enabled", "false"}});

  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto assets = GetAllUserAssets(&prefs);

  {
    const auto count = base::ranges::count_if(
        assets, [](const mojom::BlockchainTokenPtr& item) {
          return item->is_shielded == true &&
                 item->coin == mojom::CoinType::ZEC;
        });

    EXPECT_EQ(0, count);
  }

  {
    const auto count = base::ranges::count_if(
        assets, [](const mojom::BlockchainTokenPtr& item) {
          return item->is_shielded == false &&
                 item->coin == mojom::CoinType::ZEC;
        });

    EXPECT_EQ(2, count);
  }
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

TEST(BraveWalletUtilsUnitTest, GetAllUserAssets) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto assets = GetAllUserAssets(&prefs);
  EXPECT_EQ(23u, assets.size());
  for (auto& asset : assets) {
    EXPECT_NE(asset->name, "");
    if (asset->symbol == "BAT") {
      EXPECT_EQ(asset->contract_address,
                "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
      EXPECT_TRUE(asset->is_erc20);
    } else {
      EXPECT_EQ(asset->contract_address, "");
      EXPECT_FALSE(asset->is_erc20);
    }
    EXPECT_FALSE(asset->is_erc721);
    EXPECT_FALSE(asset->is_erc1155);
    EXPECT_FALSE(asset->is_nft);
    EXPECT_FALSE(asset->is_spam);
    EXPECT_NE(asset->symbol, "");
    EXPECT_GT(asset->decimals, 0);
    EXPECT_TRUE(asset->visible);
    EXPECT_EQ(asset->token_id, "");
    EXPECT_NE(asset->chain_id, "");
    EXPECT_TRUE(mojom::IsKnownEnumValue(asset->coin));
  }
}

TEST(BraveWalletUtilsUnitTest, GetUserAsset) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(23u, GetAllUserAssets(&prefs).size());
  EXPECT_EQ(GetAllUserAssets(&prefs)[1],
            GetUserAsset(&prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
                         "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "",
                         false, false, false));
  EXPECT_FALSE(GetUserAsset(
      &prefs, mojom::CoinType::SOL, mojom::kMainnetChainId,
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "", false, false, false))
      << "Coin type should match";
  EXPECT_FALSE(GetUserAsset(&prefs, mojom::CoinType::ETH, mojom::kSolanaMainnet,
                            "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "",
                            false, false, false))
      << "Chain id should match";
  EXPECT_FALSE(GetUserAsset(
      &prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "", false, false, false))
      << "Address should match";

  // Test token ID cases.
  auto erc721_token = mojom::BlockchainToken::New(
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "name1", "logo",
      false /* is_compressed */, false /* is_erc20 */, true /* is_erc721 */,
      false /* is_erc1155 */, mojom::SPLTokenProgram::kUnsupported,
      true /* is_nft */, false /* is_spam */, "SYMBOL", 8 /* decimals */,
      true /* visible */, "0x11", "" /* coingecko_id */, mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  ASSERT_TRUE(AddUserAsset(&prefs, erc721_token.Clone()));
  EXPECT_EQ(erc721_token,
            GetUserAsset(&prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
                         "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x11",
                         true, false, false));
  EXPECT_FALSE(GetUserAsset(
      &prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d", "0x22", true, false, false))
      << "Token ID should match";

  auto erc1155_token = mojom::BlockchainToken::New(
      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984", "name2", "logo",
      false /* is_compressed */, false /* is_erc20 */, false /* is_erc721 */,
      true /* is_erc1155 */, mojom::SPLTokenProgram::kUnsupported,
      true /* is_nft */, false /* is_spam */, "SYMBOL2", 8 /* decimals */,
      true /* visible */, "0x22", "" /* coingecko_id */, mojom::kMainnetChainId,
      mojom::CoinType::ETH, false);
  ASSERT_TRUE(AddUserAsset(&prefs, erc1155_token.Clone()));
  EXPECT_EQ(erc1155_token,
            GetUserAsset(&prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
                         "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984", "0x22",
                         false, true, false));
  EXPECT_FALSE(GetUserAsset(
      &prefs, mojom::CoinType::ETH, mojom::kMainnetChainId,
      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984", "0x11", false, true, false))
      << "Token ID should match";

  EXPECT_FALSE(GetUserAsset(&prefs, mojom::CoinType::ETH, mojom::kZCashMainnet,
                            "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984",
                            "0x11", false, true, true))
      << "Invalid ZEC token";

  EXPECT_EQ(GetZcashNativeShieldedToken(mojom::kZCashMainnet),
            GetUserAsset(&prefs, mojom::CoinType::ZEC, mojom::kZCashMainnet, "",
                         "", false, false, true))
      << "Invalid ZEC token";

  EXPECT_EQ(GetZcashNativeShieldedToken(mojom::kZCashTestnet),
            GetUserAsset(&prefs, mojom::CoinType::ZEC, mojom::kZCashTestnet, "",
                         "", false, false, true))
      << "Invalid ZEC token";
}

TEST(BraveWalletUtilsUnitTest, AddUserAsset) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(23u, GetAllUserAssets(&prefs).size());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();
  asset->chain_id = "0x98765";
  asset->contract_address = "0x5AAEB6053F3E94C9B9A09F33669435E7EF1BEAED";

  EnsureNativeTokenForNetwork(
      &prefs, GetTestNetworkInfo1("0x98765", mojom::CoinType::ETH));

  EXPECT_EQ(24u, GetAllUserAssets(&prefs).size());

  ASSERT_TRUE(AddUserAsset(&prefs, asset->Clone()));

  // Address gets checksum format.
  asset->contract_address = "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed";

  EXPECT_EQ(25u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Contains(Eq(std::ref(asset))));

  // Adding same asset again fails.
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));

  ASSERT_TRUE(RemoveUserAsset(&prefs, asset));

  EXPECT_EQ(24u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Not(Contains(Eq(std::ref(asset)))));

  // SPL token program is set to unsupported for non-SPL tokens.
  asset->contract_address = "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d";
  asset->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  ASSERT_TRUE(AddUserAsset(&prefs, asset.Clone()));
  asset->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  EXPECT_THAT(GetAllUserAssets(&prefs), Contains(Eq(std::ref(asset))));

  // SPL token program stay as is for SPL tokens.
  asset->contract_address = "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ";
  asset->coin = mojom::CoinType::SOL;
  asset->chain_id = mojom::kSolanaMainnet;
  asset->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  ASSERT_TRUE(AddUserAsset(&prefs, asset.Clone()));
  EXPECT_THAT(GetAllUserAssets(&prefs), Contains(Eq(std::ref(asset))));

  // Invalid contract address is rejected.
  asset->coin = mojom::CoinType::ETH;
  asset->chain_id = mojom::kMainnetChainId;
  asset->contract_address = "not_eth_address";
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));

  // Invalid contract address is rejected.
  asset->coin = mojom::CoinType::SOL;
  asset->chain_id = mojom::kSolanaMainnet;
  asset->contract_address = "not_base58_encoded_string";
  ASSERT_FALSE(AddUserAsset(&prefs, asset->Clone()));
}

TEST(BraveWalletUtilsUnitTest, EnsureNativeTokenForNetwork) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(23u, GetAllUserAssets(&prefs).size());

  auto network_info = GetTestNetworkInfo1("0x98765");
  EnsureNativeTokenForNetwork(&prefs, network_info);

  EXPECT_EQ(GetAllUserAssets(&prefs).back()->chain_id, "0x98765");
  EXPECT_EQ(GetAllUserAssets(&prefs).back()->contract_address, "");
  EXPECT_EQ(GetAllUserAssets(&prefs).back()->decimals, 11);

  network_info.decimals = 55;
  EnsureNativeTokenForNetwork(&prefs, network_info);
  EXPECT_EQ(GetAllUserAssets(&prefs).back()->chain_id, "0x98765");
  EXPECT_EQ(GetAllUserAssets(&prefs).back()->contract_address, "");
  EXPECT_EQ(GetAllUserAssets(&prefs).back()->decimals, 55);
}

TEST(BraveWalletUtilsUnitTest, RemoveUserAsset) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  EXPECT_EQ(23u, GetAllUserAssets(&prefs).size());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_TRUE(RemoveUserAsset(&prefs, asset));
  EXPECT_EQ(22u, GetAllUserAssets(&prefs).size());
  EXPECT_THAT(GetAllUserAssets(&prefs), Not(Contains(Eq(std::ref(asset)))));

  asset->chain_id = "0x98765";
  EXPECT_FALSE(RemoveUserAsset(&prefs, asset));
}

TEST(BraveWalletUtilsUnitTest, SetUserAssetVisible) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_TRUE(asset->visible);

  EXPECT_TRUE(SetUserAssetVisible(&prefs, asset, false));
  asset->visible = false;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  EXPECT_TRUE(SetUserAssetVisible(&prefs, asset, true));
  asset->visible = true;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  asset->chain_id = "0x98765";
  EXPECT_FALSE(SetUserAssetVisible(&prefs, asset, false));
}

TEST(BraveWalletUtilsUnitTest, SetAssetSpamStatus) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto asset = GetAllUserAssets(&prefs)[4]->Clone();

  EXPECT_FALSE(asset->is_spam);
  EXPECT_TRUE(asset->visible);

  EXPECT_TRUE(SetAssetSpamStatus(&prefs, asset, true));
  asset->is_spam = true;
  asset->visible = false;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  EXPECT_TRUE(SetAssetSpamStatus(&prefs, asset, false));
  asset->is_spam = false;
  asset->visible = true;
  EXPECT_EQ(asset, GetAllUserAssets(&prefs)[4]->Clone());

  asset->chain_id = "0x98765";
  EXPECT_FALSE(SetAssetSpamStatus(&prefs, asset, true));
}

TEST(BraveWalletUtilsUnitTest, SetAssetSPLTokenProgram) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  auto asset = mojom::BlockchainToken::New(
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "TSLA", "tsla.png", false,
      false, false, false, mojom::SPLTokenProgram::kUnknown, false, false,
      "TSLA", 8, true, "", "", mojom::kSolanaMainnet, mojom::CoinType::SOL,
      false);
  ASSERT_TRUE(AddUserAsset(&prefs, asset->Clone()));

  ASSERT_TRUE(
      SetAssetSPLTokenProgram(&prefs, asset, mojom::SPLTokenProgram::kToken));
  asset->spl_token_program = mojom::SPLTokenProgram::kToken;
  EXPECT_EQ(asset,
            GetUserAsset(&prefs, mojom::CoinType::SOL, mojom::kSolanaMainnet,
                         "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "",
                         false, false, false));

  EXPECT_TRUE(SetAssetSPLTokenProgram(&prefs, asset,
                                      mojom::SPLTokenProgram::kToken2022));
  asset->spl_token_program = mojom::SPLTokenProgram::kToken2022;
  EXPECT_EQ(asset,
            GetUserAsset(&prefs, mojom::CoinType::SOL, mojom::kSolanaMainnet,
                         "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "",
                         false, false, false));
}

TEST(BraveWalletUtilsUnitTest, SetAssetCompressed) {
  sync_preferences::TestingPrefServiceSyncable prefs;
  RegisterProfilePrefs(prefs.registry());

  // Can't set is_compressed for ETH asset.
  auto assets = GetAllUserAssets(&prefs);
  auto eth_asset = assets[0]->Clone();
  EXPECT_FALSE(eth_asset->is_compressed);
  EXPECT_FALSE(SetAssetCompressed(&prefs, eth_asset));
  EXPECT_FALSE(GetAllUserAssets(&prefs)[0]->is_compressed);

  // Can set is_compressed for SOL asset.
  auto sol_asset = assets[13]->Clone();
  EXPECT_FALSE(sol_asset->is_compressed);
  EXPECT_TRUE(SetAssetCompressed(&prefs, sol_asset));
  EXPECT_TRUE(GetAllUserAssets(&prefs)[13]->is_compressed);
}

}  // namespace brave_wallet
