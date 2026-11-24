/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_decoder.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

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

  const std::string expected = absl::StrFormat(
      R"({
        "inputs": [{
          "txHash": "%s",
          "index": "0",
          "value": "5000000",
          "address": "%s",
          "tokens": [{"tokenId": "%s", "value": "10"}]
        }],
        "outputs": [{
          "address": "%s",
          "value": "3000000",
          "tokens": []
        }],
        "mint": [{"tokenId": "%s", "value": "7"}],
        "withdrawals": [{"address": "%s", "value": "1000000"}],
        "scriptDataHash": "%s",
        "collateral": [{
          "txHash": "%s",
          "index": "1",
          "value": null,
          "tokens": null,
          "address": null
        }],
        "collateralReturn": {
          "address": "%s",
          "value": "2000000",
          "tokens": []
        },
        "totalCollateral": "2500000"
      })",
      base::HexEncodeLower(matched_tx_hash), address1->ToString(),
      base::HexEncodeLower(token_id), address2->ToString(),
      base::HexEncodeLower(token_id), address1->ToString(),
      base::HexEncodeLower(script_data_hash),
      base::HexEncodeLower(unmatched_tx_hash), address2->ToString());

  EXPECT_THAT(details, base::test::IsJson(expected));
}

TEST(CardanoDappUtilsTest, FormatCardanoTxDetailsEmptyBody) {
  CardanoTxDecoder::SerializableTxBody tx_body;
  cardano_rpc::UnspentOutputs utxos;

  base::DictValue details = FormatCardanoTxDetails(tx_body, utxos);

  EXPECT_THAT(details, base::test::IsJson("{}"));
}

}  // namespace brave_wallet
