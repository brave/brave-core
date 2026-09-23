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
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}, 2});
    auto result =
        PickZCashOrchardInputs(notes, 10000u, ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    // max(2, spend(1) + change(1) + target(1)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 200000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
  }

  // Transparent output, single input.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 10000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    // max(2, max(0, 1) + max(spend(1) + change(1), 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 200000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
  }

  // Orchard output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});
    auto result =
        PickZCashOrchardInputs(notes, 150000u, ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, spends(2) + change(1) + target(1)) * 5000.
    EXPECT_EQ(result->fee, 20000u);
    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Full amount, orchard output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, spends(3) + target(1)) * 5000, no change output.
    EXPECT_EQ(result->fee, 20000u);

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
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 370000u - 25000u,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, spends(3) + change(1) + target(1)) * 5000. The change output is
    // assumed while picking, so it is still paid for even though change is 0.
    EXPECT_EQ(result->fee, 25000u);

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
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});

    auto result = PickZCashOrchardInputs(notes, 150000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(spends(2) + change(1), 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);

    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Transparent output, full amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 1) + max(spends(3) + outputs(0), 2)) * 5000.
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
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 370000u - 25000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 1) + max(spends(3) + change(1), 2)) * 5000. The change
    // output is assumed while picking, so it is paid for even when change is 0.
    EXPECT_EQ(result->fee, 25000u);

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
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 2});
    auto result =
        PickZCashOrchardInputs(notes, 300000u, ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Zero-amount notes are skipped so they don't inflate the Orchard action
  // count (and therefore the ZIP-317 fee) beyond what's needed to cover the
  // target amount. Matches how
  // ZCashCreateOrchardToTransparentTransactionTask calls this: two
  // value-bearing notes selected (dust filtered) needs 2 spends + 1 change
  // output = 3 actions, not max(2, 1, 2) = 2.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 0u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 3u, {}, 5000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 4u, {}, 100000u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 5u, {}, 0u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 10000u,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    // Only the two value-bearing notes are selected.
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(spends(2) + change(1), 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);
    EXPECT_EQ(result->change, 105000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs[0].amount, 5000u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
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
    notes.push_back(OrchardNote{{}, 1u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xAAAAAAAAAAAAAAAA, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 0x8888888888888888, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xAAAAAAAAAAAAAAAA, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 0x8888888888888888, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs greater than u32, full amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(
        OrchardNote{{}, 1u, {}, 0xFFFFFFFF + 100000u, 0, {}, {}, 2});
    notes.push_back(
        OrchardNote{{}, 2u, {}, 0xFFFFFFFF + 200000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kOrchard);

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, spends(2) + target(1)) * 5000, no change output.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 0xFFFFFFFF + 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 0xFFFFFFFF + 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, full amount, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(
        OrchardNote{{}, 1u, {}, 0xFFFFFFFF + 100000u, 0, {}, {}, 2});
    notes.push_back(
        OrchardNote{{}, 2u, {}, 0xFFFFFFFF + 200000u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, kZCashFullAmount,
                                         ZCashTargetOutputType::kTransparent);

    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(spends(2) + outputs(0), 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);

    EXPECT_EQ(result->inputs[0].amount, 0xFFFFFFFF + 100000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 0xFFFFFFFF + 200000u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, with change amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 4295117295u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 4295167295u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 4295117295u,
                                         ZCashTargetOutputType::kOrchard);

    EXPECT_EQ(result->change,
              notes[0].amount + notes[1].amount - 4295117295u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, spends(2) + change(1) + target(1)) * 5000.
    EXPECT_EQ(result->fee, 20000u);

    EXPECT_EQ(result->inputs[0].amount, 4295117295u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 4295167295u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }

  // Inputs greater than u32, with change amount, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 4295117295u, 0, {}, {}, 2});
    notes.push_back(OrchardNote{{}, 2u, {}, 4295167295u, 0, {}, {}, 2});
    auto result = PickZCashOrchardInputs(notes, 4295117295u,
                                         ZCashTargetOutputType::kTransparent);

    EXPECT_EQ(result->change,
              notes[0].amount + notes[1].amount - 4295117295u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(spends(2) + change(1), 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);

    EXPECT_EQ(result->inputs[0].amount, 4295117295u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
    EXPECT_EQ(result->inputs[1].amount, 4295167295u);
    EXPECT_EQ(result->inputs[1].block_id, 2u);
  }
}

TEST(ZCashTransactionUtilsUnitTest, PickZCashIronwoodInputs) {
  // No inputs, Ironwood output.
  {
    std::vector<OrchardNote> notes;
    auto result = PickZCashIronwoodInputs(notes, 10000u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_FALSE(result);
  }

  // No inputs, transparent output.
  {
    std::vector<OrchardNote> notes;
    auto result = PickZCashIronwoodInputs(notes, 10000u,
                                          ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result);
  }

  // Empty inputs, full amount.
  {
    auto result =
        PickZCashIronwoodInputs(std::vector<OrchardNote>(), kZCashFullAmount,
                                ZCashTargetOutputType::kIronwood);
    EXPECT_FALSE(result.has_value());
  }

  // Ironwood output, single input. Spends and outputs share the same bundle, so
  // the fee is max(2, max(1, 2)) * 5000 - not double-billed against a legacy
  // Orchard bundle.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 10000u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    EXPECT_EQ(result->inputs[0].amount, 200000u);
    EXPECT_EQ(result->inputs[0].block_id, 1u);
  }

  // Transparent output, single input.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 200000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 10000u,
                                          ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    // max(2, max(0, 1) + max(1, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->change, 200000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 1u);
    EXPECT_EQ(result->inputs[0].amount, 200000u);
  }

  // Ironwood output, with change.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 150000u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    // max(2, max(2, 1 + 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->change, 170000u - 150000u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    EXPECT_EQ(result->inputs[0].amount, 70000u);
    EXPECT_EQ(result->inputs[0].block_id, 3u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
    EXPECT_EQ(result->inputs[1].block_id, 1u);
  }

  // Full amount, Ironwood output - no change output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, kZCashFullAmount,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(3, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->inputs[0].amount, 100000u);
    EXPECT_EQ(result->inputs[1].amount, 200000u);
    EXPECT_EQ(result->inputs[2].amount, 70000u);
  }

  // Full amount, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, kZCashFullAmount,
                                          ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(0, 1) + max(3, 0, 2)) * 5000.
    EXPECT_EQ(result->fee, 20000u);
  }

  // Change is 0, but amount is not full, Ironwood output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 3u, {}, 70000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 370000u - 15000u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 3u);
    // max(2, max(3, 1 + 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
  }

  // Unable to pick inputs, not enough funds.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 200000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 300000u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_FALSE(result.has_value());
  }

  // Zero-amount notes are skipped so they don't inflate the Ironwood action
  // count (and therefore the ZIP-317 fee).
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 0u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 3u, {}, 5000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 4u, {}, 100000u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 5u, {}, 0u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 10000u,
                                          ZCashTargetOutputType::kTransparent);
    EXPECT_TRUE(result.has_value());
    // Only the two value-bearing notes are selected.
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(0, 1) + max(2, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 15000u);
    EXPECT_EQ(result->change, 105000u - 10000u - result->fee);
    EXPECT_EQ(result->inputs[0].amount, 5000u);
    EXPECT_EQ(result->inputs[1].amount, 100000u);
  }

  // Inputs overflow.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 0xFFFFFFFFFFFFFFFF, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, kZCashFullAmount,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs overflow, transparent output.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 0xAAAAAAAAAAAAAAAA, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 0x8888888888888888, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, kZCashFullAmount,
                                          ZCashTargetOutputType::kTransparent);
    EXPECT_FALSE(result.has_value());
  }

  // Inputs greater than u32, full amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(
        OrchardNote{{}, 1u, {}, 0xFFFFFFFF + 100000u, 0, {}, {}, 3});
    notes.push_back(
        OrchardNote{{}, 2u, {}, 0xFFFFFFFF + 200000u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, kZCashFullAmount,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change, 0u);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(2, 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs[0].amount, 0xFFFFFFFF + 100000u);
    EXPECT_EQ(result->inputs[1].amount, 0xFFFFFFFF + 200000u);
  }

  // Inputs greater than u32, with change amount.
  {
    std::vector<OrchardNote> notes;
    notes.push_back(OrchardNote{{}, 1u, {}, 4295117295u, 0, {}, {}, 3});
    notes.push_back(OrchardNote{{}, 2u, {}, 4295167295u, 0, {}, {}, 3});
    auto result = PickZCashIronwoodInputs(notes, 4295117295u,
                                          ZCashTargetOutputType::kIronwood);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->change,
              notes[0].amount + notes[1].amount - 4295117295u - result->fee);
    EXPECT_EQ(result->inputs.size(), 2u);
    // max(2, max(2, 1 + 1, 2)) * 5000.
    EXPECT_EQ(result->fee, 10000u);
    EXPECT_EQ(result->inputs[0].amount, 4295117295u);
    EXPECT_EQ(result->inputs[1].amount, 4295167295u);
  }
}

TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee) {
  // https://github.com/zcash/librustzcash/blob/e190b6b7baec244899556abed8f12f21fff19abf/zcash_client_backend/src/data_api/testing/pool.rs#L3961
  // 5000 * max(2, max(0, 1) + max(spend(1) + change(1), 2))
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 1u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());
  // 5000 * max(2, (max(1, 2))
  EXPECT_EQ(10000u,
            CalculateZCashTxFee(1u, 0u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());

  // https://3xpl.com/zcash/transaction/3f7d24396bd120ef79b893983d78fc7e28dbe1d6c208ec50cd1285ff85c52d42
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(1u, 0u, 0u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());
  // 5000 * max(2, max(spend(1) + change(1) + target(1), 2))
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 1u, 0u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());

  // 5000 * max(2, (max(5, 2))
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(5u, 0u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());
  // 5000 * max(2, max(5, 1) + max(0 + target(1), 2))
  EXPECT_EQ(35000u,
            CalculateZCashTxFee(5u, 0u, 0u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());

  // 5000 * max(2, max(0, 1) + max(spends(5) + change(1), 2))
  EXPECT_EQ(35000u,
            CalculateZCashTxFee(0u, 5u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());
  // 5000 * max(2, max(spends(5) + change(1) + target(1), 2))
  EXPECT_EQ(35000u,
            CalculateZCashTxFee(0u, 5u, 0u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());

  // 5000 * max(2, (max(5, 1) + 0)
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(5u, 0u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());

  EXPECT_FALSE(CalculateZCashTxFee(0xFFFFFFFF, 0u, 0u,
                                   ZCashTargetOutputType::kOrchard,
                                   /*has_change=*/true)
                   .IsValid());
  EXPECT_FALSE(CalculateZCashTxFee(0u, 0xFFFFFFFF, 0u,
                                   ZCashTargetOutputType::kTransparent,
                                   /*has_change=*/true)
                   .IsValid());
  EXPECT_FALSE(CalculateZCashTxFee(0u, 0u, 0xFFFFFFFF,
                                   ZCashTargetOutputType::kTransparent,
                                   /*has_change=*/true)
                   .IsValid());

  // Inputs must come from exactly one pool.
  EXPECT_DEATH_IF_SUPPORTED(
      {
        CalculateZCashTxFee(1u, 1u, 0u, ZCashTargetOutputType::kTransparent,
                            /*has_change=*/true);
      },
      "");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        CalculateZCashTxFee(0u, 1u, 1u, ZCashTargetOutputType::kTransparent,
                            /*has_change=*/true);
      },
      "");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        CalculateZCashTxFee(1u, 0u, 1u, ZCashTargetOutputType::kTransparent,
                            /*has_change=*/true);
      },
      "");
  EXPECT_DEATH_IF_SUPPORTED(
      {
        CalculateZCashTxFee(0u, 0u, 0u, ZCashTargetOutputType::kTransparent,
                            /*has_change=*/true);
      },
      "");
}

// Regression test: the legacy Orchard pool is only ever spent inside a v6
// transaction, which disables cross-address transfers, so a spend and an output
// never share an action — the action count is `spends + outputs`, not
// `max(spends, outputs)`. Undercounting this underpays the ZIP-317 fee and gets
// the tx rejected by the network as "unpaid actions". Contrast with the
// Ironwood pool, covered by CalculateZCashTxFee_IronwoodInputs.
TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee_OrchardInputs) {
  // 2 orchard spends + 1 orchard change output + 1 transparent target output.
  // Orchard actions = spends(2) + outputs(1) = 3 -> fee = 5000 * (1 + 3).
  // Were these actions to pair a spend with an output, this would be
  // max(2, 1, 2) = 2, and the tx would underpay by 5000.
  EXPECT_EQ(20000u,
            CalculateZCashTxFee(0u, 2u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());
  // Same, targeting the legacy Orchard pool: the target output joins the same
  // bundle. Actions = spends(2) + change(1) + target(1) = 4.
  EXPECT_EQ(20000u,
            CalculateZCashTxFee(0u, 2u, 0u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());
}

// Full amount sends have no change output, so they must not be charged for one.
// With the legacy Orchard pool this is visible in the fee, since that pool's
// action count is `spends + outputs` rather than `max(spends, outputs)`.
TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee_NoChange) {
  // 2 legacy orchard spends + 1 transparent target output, no change.
  // Actions = max(0, 1) + (spends(2) + outputs(0)) -> fee = 5000 * 3.
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 2u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/false)
                .ValueOrDie());
  // A single spend still pays the 2 action Orchard minimum.
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 1u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/false)
                .ValueOrDie());
  // Transparent only: actions = max(inputs(3), outputs(1)).
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(3u, 0u, 0u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/false)
                .ValueOrDie());
}

TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee_OrchardToIronwood) {
  // The legacy Orchard and Ironwood outputs belong to separate bundles.
  // Actions = legacy spends(2) + change(1) + Ironwood minimum(2).
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(0u, 2u, 0u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());
  // Full amount: no legacy Orchard change output.
  // Actions = legacy spends(2) + Ironwood minimum(2).
  EXPECT_EQ(20000u,
            CalculateZCashTxFee(0u, 2u, 0u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/false)
                .ValueOrDie());
}

// Regression test for https://github.com/brave/brave-browser/issues/58957:
// spending Ironwood notes must bill the Ironwood bundle only. Previously the
// spends were counted against the legacy Orchard bundle, so an Ironwood to
// Ironwood transfer paid for two bundles instead of one.
TEST(ZCashTransactionUtilsUnitTest, CalculateZCashTxFee_IronwoodInputs) {
  // Ironwood to Ironwood - spends, change and target all share the one Ironwood
  // bundle: actions = max(spends(2), change(1) + target(1), 2). Spending the
  // same notes used to also raise a legacy Orchard bundle, doubling this.
  EXPECT_EQ(10000u,
            CalculateZCashTxFee(0u, 0u, 2u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());
  // 5 spends dominate the change + target outputs.
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(0u, 0u, 5u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());

  // Ironwood to transparent: actions = max(0, 1) + max(spends, change, 2).
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(0u, 0u, 1u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());
  EXPECT_EQ(30000u,
            CalculateZCashTxFee(0u, 0u, 5u, ZCashTargetOutputType::kTransparent,
                                /*has_change=*/true)
                .ValueOrDie());

  // Ironwood to legacy Orchard - separate bundles.
  // Actions = max(spends(2), change(1), 2) + Orchard minimum(2).
  EXPECT_EQ(20000u,
            CalculateZCashTxFee(0u, 0u, 2u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());

  // Transparent to Ironwood: actions = max(inputs, change) + Ironwood
  // minimum(2). Matches the fee the old `kOrchard` workaround produced.
  EXPECT_EQ(15000u,
            CalculateZCashTxFee(1u, 0u, 0u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());
  EXPECT_EQ(25000u,
            CalculateZCashTxFee(3u, 0u, 0u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());
}

// Regression test for the double-bundle bug in
// https://github.com/brave/brave-browser/issues/58957: an Ironwood to Ironwood
// transfer must raise exactly one bundle. Spending Ironwood notes used to be
// billed against the legacy Orchard counter, so the spend and its change landed
// in an Orchard bundle - max(spend(1), change(1), 2) = 2 actions - while the
// target output raised a second, spendless Ironwood bundle - max(0, 1, 2) = 2
// actions - totalling 4 actions / 20000 zatoshi for a transfer that needs 2
// actions / 10000 zatoshi.
//
// The phantom bundle always costs the 2 action minimum, so the signature of the
// bug is a fee inflated by exactly 10000 zatoshi, independent of note count.
TEST(ZCashTransactionUtilsUnitTest,
     CalculateZCashTxFee_IronwoodToIronwoodSingleBundle) {
  // The case from the issue: one spend, one change output, one target output.
  // Spend and outputs pair up inside the single Ironwood bundle, so actions =
  // max(spends(1), change(1) + target(1), 2) = 2.
  EXPECT_EQ(10000u,
            CalculateZCashTxFee(0u, 0u, 1u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/true)
                .ValueOrDie());

  // Full amount send - no change output, still one bundle at the 2 action
  // minimum.
  EXPECT_EQ(10000u,
            CalculateZCashTxFee(0u, 0u, 1u, ZCashTargetOutputType::kIronwood,
                                /*has_change=*/false)
                .ValueOrDie());

  // The fee must track the spend count alone once spends exceed the minimum.
  // A phantom Orchard bundle would add a flat 10000 to every row here.
  struct {
    uint32_t ironwood_spends;
    uint64_t expected_fee;
  } const kCases[] = {
      {1u, 10000u},  // max(1, 2, 2) = 2
      {2u, 10000u},  // max(2, 2, 2) = 2
      {3u, 15000u},  // max(3, 2, 2) = 3
      {5u, 25000u},  // max(5, 2, 2) = 5
  };
  for (const auto& test_case : kCases) {
    SCOPED_TRACE(test_case.ironwood_spends);
    EXPECT_EQ(test_case.expected_fee,
              CalculateZCashTxFee(0u, 0u, test_case.ironwood_spends,
                                  ZCashTargetOutputType::kIronwood,
                                  /*has_change=*/true)
                  .ValueOrDie());
  }

  // The legacy Orchard bundle is genuinely separate, so targeting it from
  // Ironwood notes does cost two bundles. This is the fee the Ironwood to
  // Ironwood cases above must not be charged.
  EXPECT_EQ(20000u,
            CalculateZCashTxFee(0u, 0u, 1u, ZCashTargetOutputType::kOrchard,
                                /*has_change=*/true)
                .ValueOrDie());
}

}  // namespace brave_wallet
