/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/numerics/checked_math.h"
#include "base/numerics/safe_conversions.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/internal/cardano_tx_decoder.h"

namespace brave_wallet {

namespace {

// https://github.com/Emurgo/cardano-serialization-lib/blob/c8bb8f43a916d804b89c3e226560265b65f1689a/rust/src/utils.rs#L791
constexpr uint64_t kMinAdaUtxoConstantOverhead = 160;
constexpr int kFeeSearchMaxIterations = 10;

// Setup empty witness set based on number of different addresses corresponding
// to inputs.
void SetupDummyWitnessSet(CardanoTransaction& tx) {
  tx.SetWitnesses(std::vector<CardanoTransaction::TxWitness>(
      tx.GetInputAddresses().size()));
}

}  // namespace

CardanoTransactionSerializer::CardanoTransactionSerializer() = default;

// static
std::optional<std::vector<uint8_t>>
CardanoTransactionSerializer::SerializeTransaction(
    const CardanoTransaction& tx) {
  auto serializable_tx = tx.ToSerializableTx();
  if (!serializable_tx) {
    return std::nullopt;
  }
  return CardanoTxDecoder::EncodeTransaction(*serializable_tx);
}

// static
std::optional<std::array<uint8_t, kCardanoTxHashSize>>
CardanoTransactionSerializer::GetTxHash(const CardanoTransaction& tx) {
  auto serializable_tx = tx.ToSerializableTx();
  if (!serializable_tx) {
    return std::nullopt;
  }

  return CardanoTxDecoder::GetTransactionHash(*serializable_tx);
}

// static
std::optional<uint64_t> CardanoTransactionSerializer::CalcMinTransactionFee(
    const CardanoTransaction& tx,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  auto serialized_transaction =
      CardanoTransactionSerializer::SerializeTransaction(tx);
  if (!serialized_transaction) {
    return std::nullopt;
  }

  base::CheckedNumeric<uint64_t> tx_size = serialized_transaction->size();
  base::CheckedNumeric<uint64_t> fee =
      tx_size * epoch_parameters.min_fee_coefficient +
      epoch_parameters.min_fee_constant;
  if (fee.IsValid()) {
    return fee.ValueOrDie();
  }
  return std::nullopt;
}

// static
std::optional<uint64_t> CardanoTransactionSerializer::CalcRequiredCoin(
    const CardanoTransaction::TxOutput& output,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  auto cbor_bytes = CardanoTxDecoder::EncodeTransactionOutput(
      output.ToSerializableTxOutput());
  if (!cbor_bytes) {
    return std::nullopt;
  }

  uint64_t required_coin = 0;
  if (!base::CheckMul<uint64_t>(
           epoch_parameters.coins_per_utxo_size,
           base::CheckAdd<uint64_t>(cbor_bytes->size(),
                                    kMinAdaUtxoConstantOverhead))
           .AssignIfValid(&required_coin)) {
    return std::nullopt;
  }

  return required_coin;
}

// static
std::optional<uint64_t> CardanoTransactionSerializer::CalcMinAdaRequired(
    const CardanoTransaction::TxOutput& output,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  // https://github.com/Emurgo/cardano-serialization-lib/blob/c8bb8f43a916d804b89c3e226560265b65f1689a/rust/src/utils.rs#L767

  CardanoTransaction::TxOutput cur_output = output;
  // We need at most 5 iterations as uint64 can be encoded by CBOR in 1, 2, 3,
  // 5, or 9 bytes. Each iteration strictly increases the amount to the required
  // lovelace. Last iteration moved out of the loop assuming output having 9
  // bytes for amount max(uint64) produces the largest coin requirement.
  for (int i = 0; i < 4; i++) {
    auto required_coin = CalcRequiredCoin(cur_output, epoch_parameters);
    if (!required_coin) {
      return std::nullopt;
    }

    if (cur_output.amount < required_coin.value()) {
      // Current output amount is less than required lovelace. But larger
      // required lovelace may produce larger cbor binary for this output. So we
      // increase the amount and run loop again.
      cur_output.amount = required_coin.value();
    } else {
      return required_coin.value();
    }
  }
  cur_output.amount = std::numeric_limits<int64_t>::max();
  return CalcRequiredCoin(cur_output, epoch_parameters);
}

// static
bool CardanoTransactionSerializer::ValidateMinValue(
    const CardanoTransaction::TxOutput& output,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  auto min_ada_required = CardanoTransactionSerializer::CalcMinAdaRequired(
      output, epoch_parameters);
  return min_ada_required && output.amount >= min_ada_required.value();
}

// static
std::optional<CardanoTransaction>
CardanoTransactionSerializer::AdjustFeeAndOutputsForTx(
    const CardanoTransaction& base_tx,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  CardanoTransaction result = base_tx;

  base::CheckedNumeric<uint64_t> total_inputs_amount =
      result.GetTotalInputsAmount();

  // These values are not supposed to be set before.
  CHECK_EQ(result.fee(), 0u);
  if (result.ChangeOutput()) {
    CHECK_EQ(result.ChangeOutput()->amount, 0u);
  }
  if (result.sending_max_amount()) {
    CHECK_EQ(result.TargetOutput()->amount, 0u);
  }
  CHECK(result.witnesses().empty());

  // Add dummy witness set based on number of signatures we need. This ensures
  // result transaction could be encoded to its final size and we can calculate
  // correct fee for it.
  SetupDummyWitnessSet(result);

  // This is starting fee based on minimum size of tx as fee and outputs are 0.
  if (auto start_fee = CardanoTransactionSerializer::CalcMinTransactionFee(
          result, epoch_parameters)) {
    result.set_fee(*start_fee);
  } else {
    return std::nullopt;
  }

  for (int i = 0; i < kFeeSearchMaxIterations; i++) {
    // Adjust outputs based on current tx fee.
    if (result.sending_max_amount()) {
      if (!base::CheckSub(total_inputs_amount, result.fee())
               .AssignIfValid(&result.TargetOutput()->amount)) {
        return std::nullopt;
      }
    } else if (result.ChangeOutput()) {
      if (!base::CheckSub(total_inputs_amount, result.fee(),
                          result.TargetOutput()->amount)
               .AssignIfValid(&result.ChangeOutput()->amount)) {
        return std::nullopt;
      }
    }

    auto required_fee = CardanoTransactionSerializer::CalcMinTransactionFee(
        result, epoch_parameters);
    if (!required_fee) {
      return std::nullopt;
    }

    // Break search loop if required fee is less than or equal to current fee.
    // That means current tx fee is enough to cover the transaction costs(based
    // on its binary size).
    if (*required_fee <= result.fee()) {
      if (ValidateAmounts(result, epoch_parameters)) {
        // Remove dummy witness set.
        result.SetWitnesses({});
        return result;
      }
      return std::nullopt;
    }

    // Run the loop again with larger fee.
    result.set_fee(*required_fee);
  }

  return std::nullopt;
}

// static
bool CardanoTransactionSerializer::ValidateAmounts(
    const CardanoTransaction& tx,
    const cardano_rpc::EpochParameters& epoch_parameters) {
  for (auto& output : tx.outputs()) {
    if (!ValidateMinValue(output, epoch_parameters)) {
      return false;
    }
  }

  base::CheckedNumeric<uint64_t> inputs = tx.GetTotalInputsAmount();
  base::CheckedNumeric<uint64_t> outputs =
      base::CheckAdd(tx.GetTotalOutputsAmount(), tx.fee());

  return inputs.IsValid() && outputs.IsValid() &&
         inputs.ValueOrDie() == outputs.ValueOrDie();
}

}  // namespace brave_wallet
