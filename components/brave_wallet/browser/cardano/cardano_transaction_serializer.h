/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_

#include <array>
#include <vector>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "components/cbor/values.h"

namespace brave_wallet {

// Utility class for serializing Cardano transactions and related functionality.
class CardanoTransactionSerializer {
 public:
  // Serialization options flags.
  struct Options {
    // Used to estimate transaction size when target output value is not yet
    // known.
    bool max_value_for_target_output = false;
    // Used to estimate transaction size when change output value is not yet
    // known.
    bool max_value_for_change_output = false;
    // Used to estimate transaction size when transaction fee is not yet known.
    bool max_value_for_fee = false;
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

  // Calculates minimum ADA required for the output.
  std::optional<uint64_t> CalcMinAdaRequired(
      const CardanoTransaction::TxOutput& output,
      const cardano_rpc::EpochParameters& epoch_parameters);

  // Validate minimum ADA required for the output.
  bool ValidateMinValue(const CardanoTransaction::TxOutput& output,
                        const cardano_rpc::EpochParameters& epoch_parameters);

 private:
  Options options_ = {};

  cbor::Value::ArrayValue SerializeInputs(const CardanoTransaction& tx);
  cbor::Value::ArrayValue SerializeOutput(
      const CardanoTransaction::TxOutput& output);
  cbor::Value::ArrayValue SerializeOutputs(const CardanoTransaction& tx);
  cbor::Value SerializeTxBody(const CardanoTransaction& tx);
  cbor::Value SerializeWitnessSet(const CardanoTransaction& tx);
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TRANSACTION_SERIALIZER_H_
