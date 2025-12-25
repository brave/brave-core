/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_

#include <array>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"

namespace brave_wallet {

// Utility class for serializing Cardano transactions and related functionality.
class CardanoTransactionSerializer {
 public:
  CardanoTransactionSerializer();

  // Serializes a Cardano transaction into a byte vector (CBOR format).
  static std::optional<std::vector<uint8_t>> SerializeTransaction(
      const CardanoTransaction& tx);

  // Computes the transaction hash (Blake2b-256 hash of the serialized
  // transaction body)
  static std::optional<std::array<uint8_t, kCardanoTxHashSize>> GetTxHash(
      const CardanoTransaction& tx);

  // Validate minimum ADA required for the output.
  static bool ValidateMinValue(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);

  // Validate that inputs match outputs and fees. Also validates that the
  // outputs conform minimum ADA value limit.
  static bool ValidateAmounts(
      const CardanoTransaction& tx,
      const cardano_rpc::EpochParameters& epoch_parameters);

  // Based on `base_tx` find valid fee and outputs to cover the transaction
  // costs.
  static std::optional<CardanoTransaction> AdjustFeeAndOutputsForTx(
      const CardanoTransaction& base_tx,
      const cardano_rpc::EpochParameters& epoch_parameters);

  // Calculates minimum ADA required for the output.
  static std::optional<uint64_t> CalcMinAdaRequired(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest,
                           CalcMinAdaRequired);
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest, ValidateMinValue);
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest,
                           CalcMinTransactionFee);

  // Calculates minimum transaction fee based on its size and epoch parameters.
  static std::optional<uint64_t> CalcMinTransactionFee(
      const CardanoTransaction& tx,
      const cardano_rpc::EpochParameters& epoch_parameters);

  static std::optional<uint64_t> CalcRequiredCoin(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_
