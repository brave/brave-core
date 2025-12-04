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
#include "components/cbor/values.h"

namespace brave_wallet {

// Utility class for serializing Cardano transactions and related functionality.
class CardanoTransactionSerializer {
 public:
  // Serialization options flags.
  struct Options {
    // Used to estimate transaction size which is not signed yet.
    bool use_dummy_witness_set = false;
  };

  CardanoTransactionSerializer();
  explicit CardanoTransactionSerializer(Options options);

  // Serializes a Cardano transaction into a byte vector (CBOR format).
  std::vector<uint8_t> SerializeTransaction(const CardanoTransaction& tx);

  // Calculates the size (in bytes) of the serialized transaction.
  uint32_t CalcTransactionSize(const CardanoTransaction& tx);

  // Computes the transaction hash (Blake2b-256 hash of the serialized
  // transaction body)
  std::array<uint8_t, kCardanoTxHashSize> GetTxHash(
      const CardanoTransaction& tx);

  // Calculates minimum transaction fee based on its size and epoch parameters.
  uint64_t CalcMinTransactionFee(
      const CardanoTransaction& tx,
      const cardano_rpc::EpochParameters& epoch_parameters);

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

 private:
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest,
                           CalcMinAdaRequired);
  FRIEND_TEST_ALL_PREFIXES(CardanoTransactionSerializerTest, ValidateMinValue);

  // Calculates minimum ADA required for the output.
  std::optional<uint64_t> CalcMinAdaRequired(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);

  Options options_ = {};

  cbor::Value::ArrayValue SerializeInputs(const CardanoTransaction& tx);
  cbor::Value::ArrayValue SerializeOutput(
      const CardanoTransaction::TxOutput& output);
  cbor::Value::ArrayValue SerializeOutputs(const CardanoTransaction& tx);
  cbor::Value SerializeTxBody(const CardanoTransaction& tx);
  cbor::Value SerializeWitnessSet(const CardanoTransaction& tx);

  std::optional<uint64_t> CalcRequiredCoin(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_
