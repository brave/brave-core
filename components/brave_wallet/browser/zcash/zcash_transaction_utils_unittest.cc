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
  // No inputs, transparent output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    auto result = PickZCashTransparentInputs(
        utxo_map, 30000u, ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result);
  }

  // No inputs, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    auto result = PickZCashTransparentInputs(utxo_map, 30000u,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result);
  }

  // Signle input, transparent output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["100000"] = GetZCashUtxo(100000);

    auto result = PickZCashTransparentInputs(
        utxo_map, 30000u, ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->change, 100000u - 30000u - result->fee);
    // max(2, max(1, 1)) * 5000.
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "100000");
    EXPECT_EQ(result->inputs[0].utxo_value, 100000u);
  }

  // Signle input, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["100000"] = GetZCashUtxo(100000);

    auto result = PickZCashTransparentInputs(utxo_map, 30000,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result);
    // max(2, max(1, 1) + max(1, 0, 2)) * 5000.
    EXPECT_EQ(result->change, 100000u - 30000u - 15000u);
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "100000");
    EXPECT_EQ(result->inputs[0].utxo_value, 100000u);
  }

  // Full amount, but fee is greater.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["1"] = GetZCashUtxo(1);
    utxo_map["2"] = GetZCashUtxo(2);
    utxo_map["3"] = GetZCashUtxo(3);
    auto result = PickZCashTransparentInputs(
        utxo_map, kZCashFullAmount, ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result);
  }

  // Full amount.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    auto result = PickZCashTransparentInputs(
        utxo_map, kZCashFullAmount, ZCashTargetOutputType::kTransparent);
    EXPECT_EQ(result->change, 0u);
    // max(2, max(3, 1)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // Change is 0, but amount is not max.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    auto result = PickZCashTransparentInputs(
        utxo_map, 60000u - 15000u, ZCashTargetOutputType::kTransparent);
    EXPECT_EQ(result->change, 0u);
    // max(2, max(3, 0)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // Change is 0, but amount is not full, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    auto result = PickZCashTransparentInputs(utxo_map, 60000u - 25000u,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_EQ(result->change, 0u);
    // max(2, max(3, 1) + max(0, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 25000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // With change, transparent output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    utxo_map["40000"] = GetZCashUtxo(40000);

    auto result = PickZCashTransparentInputs(
        utxo_map, 30000, ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result);
    // max(2, max(3, 1)) * 5000.
    EXPECT_EQ(result->change, 15000u);
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // With change, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    utxo_map["40000"] = GetZCashUtxo(40000);

    auto result = PickZCashTransparentInputs(utxo_map, 30000,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->change, 5000u);
    // max(2, max(3, 1) + max(0, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 25000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // Full amount, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["10000"] = GetZCashUtxo(10000);
    utxo_map["20000"] = GetZCashUtxo(20000);
    utxo_map["30000"] = GetZCashUtxo(30000);
    auto result = PickZCashTransparentInputs(utxo_map, kZCashFullAmount,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_EQ(result->change, 0u);
    // max(2, max(3, 0) + max(0, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 25000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "10000");
    EXPECT_EQ(result->inputs[0].utxo_value, 10000u);

    EXPECT_EQ(result->inputs[1].utxo_address, "20000");
    EXPECT_EQ(result->inputs[1].utxo_value, 20000u);

    EXPECT_EQ(result->inputs[2].utxo_address, "30000");
    EXPECT_EQ(result->inputs[2].utxo_value, 30000u);
  }

  // Not enough funds.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["1"] = GetZCashUtxo(1);
    utxo_map["2"] = GetZCashUtxo(2);
    utxo_map["3"] = GetZCashUtxo(3);
    auto result = PickZCashTransparentInputs(utxo_map, 10,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result);
  }

  // Inputs greater than u32, transparent output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["4294967295"] = GetZCashUtxo(4294967295u);
    utxo_map["4294967296"] = GetZCashUtxo(4294967296u);
    utxo_map["4294967297"] = GetZCashUtxo(4294967297u);
    utxo_map["4294967298"] = GetZCashUtxo(4294967298u);

    auto result = PickZCashTransparentInputs(
        utxo_map, 4295067295u, ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->change,
              4294967295u + 4294967296u - 4295067295u - result->fee);
    // max(2, max(2, 2)) * 5000.
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "4294967295");
    EXPECT_EQ(result->inputs[0].utxo_value, 4294967295u);

    EXPECT_EQ(result->inputs[1].utxo_address, "4294967296");
    EXPECT_EQ(result->inputs[1].utxo_value, 4294967296u);
  }

  // Inputs greater that u32, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["4294967295"] = GetZCashUtxo(4294967295u);
    utxo_map["4294967296"] = GetZCashUtxo(4294967296u);
    utxo_map["4294967297"] = GetZCashUtxo(4294967297u);
    utxo_map["4294967298"] = GetZCashUtxo(4294967298u);

    auto result = PickZCashTransparentInputs(utxo_map, 4295067295u,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result);
    EXPECT_EQ(result->change,
              4294967295u + 4294967296u - 4295067295u - result->fee);
    // max(2, max(2, 1) + max(1, 0, 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);
    EXPECT_EQ(result->inputs[0].utxo_address, "4294967295");
    EXPECT_EQ(result->inputs[0].utxo_value, 4294967295u);

    EXPECT_EQ(result->inputs[1].utxo_address, "4294967296");
    EXPECT_EQ(result->inputs[1].utxo_value, 4294967296u);
  }

  // Overflow check, transparent output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["18446744073709551615"] = GetZCashUtxo(18446744073709551615u);
    utxo_map["10000"] = GetZCashUtxo(10000);
    auto result = PickZCashTransparentInputs(
        utxo_map, kZCashFullAmount, ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result);
  }

  // Overflow check, Orchard output.
  {
    ZCashWalletService::UtxoMap utxo_map;
    utxo_map["18446744073709551615"] = GetZCashUtxo(18446744073709551615u);
    utxo_map["10000"] = GetZCashUtxo(10000);
    auto result = PickZCashTransparentInputs(utxo_map, kZCashFullAmount,
                                             ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result);
  }
}

#if BUILDFLAG(ENABLE_ORCHARD)
TEST(ZCashTransactionUtilsUnitTest, PickZCashOrchardInputs) {
  // No inputs, Orchard output.
  {
    std::vector<OrchardNote> notes;
    auto result =
        PickZCashOrchardInputs(notes, 10000u, ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result);
  }

  // No inputs, transparent output.
  {
    std::vector<OrchardNote> notes;
    auto result =
        PickZCashOrchardInputs(notes, 10000u, ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result);
  }

  // Orchard output, single input.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}});
    auto result =
        PickZCashOrchardInputs(notes, 10000u, ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    // max(2, max(1, 1, 2))
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs.size(), 1u);

    EXPECT_EQ(result->inputs[0].amount, 200000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
  }

  // Transparent output, single input.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 10000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    // max(2, max(1, 0) + max(1, 0, 2))
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs.size(), 1u);

    EXPECT_EQ(result->inputs[0].amount, 200000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
  }

  // Orchard output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result =
        PickZCashOrchardInputs(notes, 150000u, ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Full amount, orchard output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
    EXPECT_EQ(result->inputs[2].amount, 70000u);
    EXPECT_EQ(result->inputs[2].block_id, 3u);
  }

  // Change is 0, but amount is not full, orchard output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 370000u - 15000u,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 0) + max(3, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
    EXPECT_EQ(result->inputs[2].amount, 200000u);
    EXPECT_EQ(result->inputs[2].block_id, 2u);
  }

  // Transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});

    auto result = PickZCashOrchardInputs(notes, 150000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(2, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Transparent output, full amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 1) + max(3, 0, 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);

    EXPECT_EQ(result->inputs[0].amount, 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
    EXPECT_EQ(result->inputs[2].amount, 70000u);
    EXPECT_EQ(result->inputs[2].block_id, 3u);
  }

  // Change is 0, but amount is not max, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 370000u - 20000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 1) + max(3, 0, 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);

    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
    EXPECT_EQ(result->inputs[2].amount, 200000u);
    EXPECT_EQ(result->inputs[2].block_id, 2u);
  }

  // Unable to pick inputs, not enough funds.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}});
    auto result =
        PickZCashOrchardInputs(notes, 300000u, ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Empty inputs, full amount.
  {
    auto result =
        PickZCashOrchardInputs(std::vector<OrchardNote>(), kZCashFullAmount,
                               ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Empty inputs, Orchard output.
  {
    auto result = PickZCashOrchardInputs(std::vector<OrchardNote>(), 10000u,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Empty inputs, Transparent output.
  {
    auto result = PickZCashOrchardInputs(std::vector<OrchardNote>(), 10000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xAAAAAAAAAAAAAAAA, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 0x8888888888888888, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xAAAAAAAAAAAAAAAA, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 0x8888888888888888, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs greater than u32, full amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xFFFFFFFF + 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 0xFFFFFFFF + 200000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(2, 0)) * 5000.
    EXPECT_EQ(result->fee, 10000u);

    EXPECT_EQ(result->inputs[0].amount, 0xFFFFFFFF + 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 0xFFFFFFFF + 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, full amount, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xFFFFFFFF + 100000u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 0xFFFFFFFF + 200000u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(1, 0) + max(2, 0, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 0xFFFFFFFF + 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 0xFFFFFFFF + 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, with change amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 4295117295u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 4295167295u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 4295117295u,
                                         ZCashTargetOutputType::kOrchard);

    EXPECT_EQ(result->change,
              notes[0].amount + notes[1].amount - 4295117295u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(2, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 10000u);

    EXPECT_EQ(result->inputs[0].amount, 4295117295u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 4295167295u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, with change amount, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 4295117295u, 0, {}, {}});
    notes.push_back(OrchardNote{{}, 2u, {}, 4295167295u, 0, {}, {}});
    auto result = PickZCashOrchardInputs(notes, 4295117295u,
                                         ZCashTargetOutputType::kTransparent);

    EXPECT_EQ(result->change,
              notes[0].amount + notes[1].amount - 4295117295u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(1,0) + max(1, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 4295117295u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 4295167295u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee) {
  // https://github.com/zcash/librustzcash/blob/e190b6b7baec244899556abed8f12f21fff19abf/zcash_client_backend/src/data_api/testing/pool.rs#L3961
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 1u, ZCashTargetOutputType::kTransparent)
                .ValueOrDie());
  // 5000 * max(2, (max(1, 2))
  EXPECT_EQ(10000u,
            CalculateZCashTxFee(1u, 0u, ZCashTargetOutputType::kTransparent)
                .ValueOrDie());

  // https://3xpl.com/zcash/transaction/3f7d24396bd120ef79b893983d78fc7e28dbe1d6c208ec50cd1285ff85c52d42
  EXPECT_EQ(15000u, CalculateZCashTxFee(1u, 0u, ZCashTargetOutputType::kOrchard)
                        .ValueOrDie());
  // 5000 * max(2, (max(0, 0) + max(1, 2, 2))
  EXPECT_EQ(10000u, CalculateZCashTxFee(0u, 1u, ZCashTargetOutputType::kOrchard)
                        .ValueOrDie());

  // 5000 * max(2, (max(5, 2))
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(5u, 0u, ZCashTargetOutputType::kTransparent)
                .ValueOrDie());
  // 5000 * max(2, (max(5, 1) + max(0, 1, 2))
  EXPECT_EQ(35000u, CalculateZCashTxFee(5u, 0u, ZCashTargetOutputType::kOrchard)
                        .ValueOrDie());

  // 5000 * max(2, (max(0, 1) + max(5u, 1, 2))
  EXPECT_EQ(30000u,
            CalculateZCashTxFee(0u, 5u, ZCashTargetOutputType::kTransparent)
                .ValueOrDie());
  // 5000 * max(2, (max(5u, 2u, 2))
  EXPECT_EQ(25000u, CalculateZCashTxFee(0u, 5u, ZCashTargetOutputType::kOrchard)
                        .ValueOrDie());

  // 5000 * max(2, (max(5, 1) + 0)
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(5u, 0u, ZCashTargetOutputType::kTransparent)
                .ValueOrDie());

  EXPECT_FALSE(
      CalculateZCashTxFee(0xFFFFFFFF, 0u, ZCashTargetOutputType::kOrchard)
          .IsValid());
  EXPECT_FALSE(
      CalculateZCashTxFee(0u, 0xFFFFFFFF, ZCashTargetOutputType::kTransparent)
          .IsValid());

  EXPECT_DEATH_IF_SUPPORTED(
      { CalculateZCashTxFee(1u, 1u, ZCashTargetOutputType::kTransparent); },
      "");
}

}  // namespace brave_wallet
