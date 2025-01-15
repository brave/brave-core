/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

#include <utility>

#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

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

#if BUILDFLAG(ENABLE_ORCHARD)
TEST(ZCashTransactionUtilsUnitTest, PickZCashOrchardInputs) {
  // Able to select inputs
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 150000u);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);  // 17 - 15
    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Full amount
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 0u);  // 17 - 15
    EXPECT_EQ(result->inputs.size(), 3u);
    EXPECT_EQ(result->fee, 20000u);
    EXPECT_EQ(result->inputs[0].amount, 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
    EXPECT_EQ(result->inputs[2].amount, 70000u);
    EXPECT_EQ(result->inputs[2].block_id, 3u);
  }

  // Unable to pick inputs
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 300000u);
    EXPECT_FALSE(result.has_value());
  }

  // Empty inputs
  {
    auto result =
        PickZCashOrchardInputs(std::vector<OrchardNote>(), kZCashFullAmount);
    EXPECT_FALSE(result.has_value());
  }

  // Empty inputs
  {
    auto result = PickZCashOrchardInputs(std::vector<OrchardNote>(), 10000u);
    EXPECT_FALSE(result.has_value());
  }
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
