/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_decoder.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

std::array<uint8_t, kCardanoTxHashSize> MakeTxHash(uint8_t fill) {
  std::array<uint8_t, kCardanoTxHashSize> hash = {};
  hash.fill(fill);
  return hash;
}

}  // namespace

TEST(CardanoDappUtilsTest, FormatCardanoTxDetails) {
  auto address1 = CardanoAddress::FromString(kMockCardanoAddress1);
  ASSERT_TRUE(address1);
  auto address2 = CardanoAddress::FromString(kMockCardanoAddress2);
  ASSERT_TRUE(address2);

  auto matched_tx_hash = MakeTxHash(0x11);
  auto unmatched_tx_hash = MakeTxHash(0x22);

  auto token_id = GetMockTokenId("TOKEN");

  // A single known utxo, matching the transaction's input by outpoint.
  cardano_rpc::UnspentOutput utxo(*address1);
  utxo.tx_hash = matched_tx_hash;
  utxo.output_index = 0;
  utxo.coin_value.lovelace_amount = 5000000;
  utxo.coin_value.tokens[token_id] = 10;
  cardano_rpc::UnspentOutputs utxos;
  utxos.push_back(std::move(utxo));

  CardanoTxDecoder::SerializableTxBody tx_body;

  // Matched input, resolved against `utxos`.
  tx_body.inputs.emplace_back(matched_tx_hash, 0);

  // Output paying to `address2`.
  tx_body.outputs.emplace_back(
      address2->ToCborBytes(),
      cardano_rpc::CoinValue(3000000, cardano_rpc::Tokens()));

  // Mint of a single token.
  tx_body.mint.emplace_back(token_id, 7);

  // Withdrawal of staking rewards to `address1`.
  tx_body.withdrawals.emplace_back(address1->ToCborBytes(), 1000000);

  auto script_data_hash = MakeTxHash(0x33);
  tx_body.script_data_hash = script_data_hash;

  // Collateral input that is NOT present in `utxos`, to exercise the
  // "unknown utxo" (null value/tokens/address) path.
  tx_body.collateral.emplace_back(unmatched_tx_hash, 1);

  tx_body.collateral_return = CardanoTxDecoder::SerializableTxOutput(
      address2->ToCborBytes(),
      cardano_rpc::CoinValue(2000000, cardano_rpc::Tokens()));
  tx_body.total_collateral = 2500000;

  base::DictValue details = FormatCardanoTxDetails(tx_body, utxos);

  // Inputs.
  auto* inputs = details.FindList("inputs");
  ASSERT_TRUE(inputs);
  ASSERT_EQ(inputs->size(), 1u);
  auto& input_value = (*inputs)[0].GetDict();
  EXPECT_EQ(*input_value.FindString("txHash"),
            base::HexEncodeLower(matched_tx_hash));
  EXPECT_EQ(*input_value.FindString("index"), "0");
  EXPECT_EQ(*input_value.FindString("value"), "5000000");
  EXPECT_EQ(*input_value.FindString("address"), address1->ToString());
  auto* input_tokens = input_value.FindList("tokens");
  ASSERT_TRUE(input_tokens);
  ASSERT_EQ(input_tokens->size(), 1u);
  EXPECT_EQ(*(*input_tokens)[0].GetDict().FindString("tokenId"),
            base::HexEncodeLower(token_id));
  EXPECT_EQ(*(*input_tokens)[0].GetDict().FindString("value"), "10");

  // Outputs.
  auto* outputs = details.FindList("outputs");
  ASSERT_TRUE(outputs);
  ASSERT_EQ(outputs->size(), 1u);
  auto& output_value = (*outputs)[0].GetDict();
  EXPECT_EQ(*output_value.FindString("address"), address2->ToString());
  EXPECT_EQ(*output_value.FindString("value"), "3000000");
  ASSERT_TRUE(output_value.FindList("tokens"));
  EXPECT_TRUE(output_value.FindList("tokens")->empty());

  // Mint.
  auto* mint = details.FindList("mint");
  ASSERT_TRUE(mint);
  ASSERT_EQ(mint->size(), 1u);
  EXPECT_EQ(*(*mint)[0].GetDict().FindString("tokenId"),
            base::HexEncodeLower(token_id));
  EXPECT_EQ(*(*mint)[0].GetDict().FindString("value"), "7");

  // Withdrawals.
  auto* withdrawals = details.FindList("withdrawals");
  ASSERT_TRUE(withdrawals);
  ASSERT_EQ(withdrawals->size(), 1u);
  EXPECT_EQ(*(*withdrawals)[0].GetDict().FindString("address"),
            address1->ToString());
  EXPECT_EQ(*(*withdrawals)[0].GetDict().FindString("value"), "1000000");

  // Script data hash.
  EXPECT_EQ(*details.FindString("scriptDataHash"),
            base::HexEncodeLower(script_data_hash));

  // Collateral (unmatched utxo -> null value/tokens/address).
  auto* collateral = details.FindList("collateral");
  ASSERT_TRUE(collateral);
  ASSERT_EQ(collateral->size(), 1u);
  auto& collateral_value = (*collateral)[0].GetDict();
  EXPECT_EQ(*collateral_value.FindString("txHash"),
            base::HexEncodeLower(unmatched_tx_hash));
  EXPECT_EQ(*collateral_value.FindString("index"), "1");
  EXPECT_FALSE(collateral_value.FindString("value"));
  EXPECT_FALSE(collateral_value.FindList("tokens"));
  EXPECT_FALSE(collateral_value.FindString("address"));

  // Collateral return.
  auto* collateral_return = details.FindDict("collateralReturn");
  ASSERT_TRUE(collateral_return);
  EXPECT_EQ(*collateral_return->FindString("address"), address2->ToString());
  EXPECT_EQ(*collateral_return->FindString("value"), "2000000");

  // Total collateral.
  EXPECT_EQ(*details.FindString("totalCollateral"), "2500000");
}

TEST(CardanoDappUtilsTest, FormatCardanoTxDetailsEmptyBody) {
  CardanoTxDecoder::SerializableTxBody tx_body;
  cardano_rpc::UnspentOutputs utxos;

  base::DictValue details = FormatCardanoTxDetails(tx_body, utxos);

  EXPECT_FALSE(details.FindList("inputs"));
  EXPECT_FALSE(details.FindList("outputs"));
  EXPECT_FALSE(details.FindList("mint"));
  EXPECT_FALSE(details.FindList("withdrawals"));
  EXPECT_FALSE(details.FindString("scriptDataHash"));
  EXPECT_FALSE(details.FindList("collateral"));
  EXPECT_FALSE(details.FindDict("collateralReturn"));
  EXPECT_FALSE(details.FindString("totalCollateral"));
}

}  // namespace brave_wallet
