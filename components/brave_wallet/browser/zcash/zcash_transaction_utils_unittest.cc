/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

#include <utility>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::vector<zcash::mojom::ZCashUtxoPtr> GetZCashUtxo(size_t seed) {
  auto utxo = zcash::mojom::ZCashUtxo::New();
  utxo->address = base::NumberToString(seed);
  utxo->value_zat = seed;
  utxo->tx_id = std::vector<uint8_t>(32u, 1u);
  std::vector<zcash::mojom::ZCashUtxoPtr> result;
  result.push_back(std::move(utxo));
  return result;
}

}  // namespace

TEST(ZCashTransactionUtilsUnitTest, PickZCashTransparentInputs) {
  // Max amount, but fee is greater
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["1"] = GetZCashUtxo(1);
    utxo_map["2"] = GetZCashUtxo(2);
    utxo_map["3"] = GetZCashUtxo(3);
    auto result = PickZCashTransparentInputs(utxo_map, kZCashFullAmount, 2);
    EXPECT_FALSE(result);
  }

  // Max amount
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    auto result = PickZCashTransparentInputs(utxo_map, kZCashFullAmount, 2);
    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->fee, CalculateZCashTxFee(3, 2));
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // Not enough funds
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["1"] = GetZCashUtxo(1);
    utxo_map["2"] = GetZCashUtxo(2);
    utxo_map["3"] = GetZCashUtxo(3);
    auto result = PickZCashTransparentInputs(utxo_map, 10, 2);
    EXPECT_FALSE(result);
  }

  // With change
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    utxo_map["40000"] = GetZCashUtxo(40000);

    auto result = PickZCashTransparentInputs(utxo_map, 30000, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->change, 15000u);
    EXPECT_EQ(result->fee, CalculateZCashTxFee(3, 0));
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }
}

}  // namespace brave_wallet
