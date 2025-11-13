/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

#include <algorithm>
#include <utility>

#include "base/numerics/checked_math.h"

namespace brave_wallet {

namespace {

constexpr uint32_t kMinOrchardActionsCountForFee = 2u;

base::CheckedNumeric<uint64_t> CalculateInputsAmount(
    const std::vector<ZCashTransaction::TxInput>& inputs) {
  base::CheckedNumeric<uint64_t> total_value = 0u;
  for (const auto& input : inputs) {
    total_value += input.utxo_value;
  }
  return total_value;
}

#if BUILDFLAG(ENABLE_ORCHARD)

base::CheckedNumeric<uint64_t> CalculateInputsAmount(
    const std::vector<OrchardNote>& notes) {
  base::CheckedNumeric<uint64_t> total_value = 0u;
  for (const auto& note : notes) {
    total_value += note.amount;
  }
  return total_value;
}

#endif  // BUILDFLAG(ENABLE_ORCHARD)

// https://github.com/zcash/orchard/blob/9d89b504c52dc69064ca431e8311a4cd1c279b44/src/builder.rs#L93-L94
base::CheckedNumeric<uint32_t> GetOrchardActionsCount(
    const base::StrictNumeric<uint32_t> orchard_input_notes,
    const base::StrictNumeric<uint32_t> orchard_output_notes) {
  if (orchard_input_notes == 0u && orchard_output_notes == 0u) {
    return 0u;
  }

  return base::CheckMax<uint32_t>(orchard_input_notes, orchard_output_notes,
                                  kMinOrchardActionsCountForFee);
}

}  // namespace

// https://zips.z.cash/zip-0317
// We assume change always exists since it doesn't affect final result:
// t->t:
// fee = max(2, (inputs, 1 + change?)) * 5000
// t->s
// fee = max(2, (inputs, change?) + max(1, 0, 2)) * 5000
// s->t
// fee = max(2, (0, 1) + max(inputs, change?, 2)) * 5000
// s->s
// fee = max(2, max(inputs, 1 + change?, 2)) * 5000.
base::CheckedNumeric<uint64_t> CalculateZCashTxFee(
    const base::StrictNumeric<uint32_t> transparent_input_count,
    const base::StrictNumeric<uint32_t> orchard_input_count,
    ZCashTargetOutputType output_type) {
  // Mixed inputs are not supported.
  CHECK((transparent_input_count != 0) ^ (orchard_input_count != 0));

  // Basic outputs setup - add a change output.
  base::CheckedNumeric<uint32_t> orchard_output_count =
      orchard_input_count != 0u ? 1u : 0u;
  base::CheckedNumeric<uint32_t> transparent_output_count =
      transparent_input_count != 0u ? 1u : 0u;

  // Add a target output.
  switch (output_type) {
    case ZCashTargetOutputType::kTransparent:
      transparent_output_count++;
      break;
    case ZCashTargetOutputType::kOrchard:
      orchard_output_count++;
      break;
    default:
      NOTREACHED();
  }

  base::CheckedNumeric<uint32_t> orchard_actions_count = GetOrchardActionsCount(
      orchard_input_count, orchard_output_count.ValueOrDie());
  // https://github.com/zcash/librustzcash/blob/8eb78dfae38ca1c91a108a86a4a3b5505766c3f6/zcash_primitives/src/transaction/fees/zip317.rs#L188
  base::CheckedNumeric<uint32_t> logical_actions_count =
      base::CheckMax<uint32_t>(transparent_input_count,
                               transparent_output_count) +
      orchard_actions_count;
  return base::CheckMul<uint64_t>(
      kMarginalFee, base::CheckMax(kGraceActionsCount, logical_actions_count));
}

PickInputsResult::PickInputsResult(
    std::vector<ZCashTransaction::TxInput> inputs,
    uint64_t fee,
    uint64_t change)
    : inputs(inputs), fee(fee), change(change) {}
PickInputsResult::~PickInputsResult() = default;
PickInputsResult::PickInputsResult(const PickInputsResult& other) = default;
PickInputsResult::PickInputsResult(PickInputsResult&& other) = default;

std::optional<PickInputsResult> PickZCashTransparentInputs(
    const ZCashWalletService::UtxoMap& utxo_map,
    uint64_t amount,
    ZCashTargetOutputType output_type) {
  if (utxo_map.empty()) {
    return std::nullopt;
  }

  // TODO(cypt4): This just picks ouputs one by one and stops when picked
  // amount is GE to send amount plus fee. Needs something better than such
  // greedy strategy.
  std::vector<ZCashTransaction::TxInput> all_inputs;
  for (const auto& item : utxo_map) {
    for (const auto& utxo : item.second) {
      if (!utxo) {
        return std::nullopt;
      }
      if (auto input =
              ZCashTransaction::TxInput::FromRpcUtxo(item.first, *utxo)) {
        all_inputs.emplace_back(std::move(*input));
      }
    }
  }

  if (amount == kZCashFullAmount) {
    auto total_inputs_amount = CalculateInputsAmount(all_inputs);
    // Full amount case - no change output.
    base::CheckedNumeric<uint64_t> fee = CalculateZCashTxFee(
        base::checked_cast<uint32_t>(all_inputs.size()), 0u, output_type);
    if (!fee.IsValid() || !total_inputs_amount.IsValid()) {
      return std::nullopt;
    }
    // Check whether total_inputs_amount amount is not less than fee
    if (!base::CheckSub(total_inputs_amount, fee).IsValid()) {
      return std::nullopt;
    }
    return PickInputsResult{std::move(all_inputs), fee.ValueOrDie(), 0u};
  }

  std::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  std::vector<ZCashTransaction::TxInput> selected_inputs;
  for (auto& input : all_inputs) {
    selected_inputs.push_back(std::move(input));

    base::CheckedNumeric<uint64_t> fee = CalculateZCashTxFee(
        base::checked_cast<uint32_t>(selected_inputs.size()), 0u, output_type);

    auto total_inputs_amount = CalculateInputsAmount(selected_inputs);
    if (!fee.IsValid() || !total_inputs_amount.IsValid()) {
      return std::nullopt;
    }

    auto amount_and_fee = base::CheckAdd<uint64_t>(amount, fee);
    if (!amount_and_fee.IsValid()) {
      return std::nullopt;
    }

    auto change = base::CheckSub(total_inputs_amount, amount_and_fee);

    if (change.IsValid()) {
      return PickInputsResult{std::move(selected_inputs), fee.ValueOrDie(),
                              change.ValueOrDie()};
    }
  }

  return std::nullopt;
}

PickOrchardInputsResult::PickOrchardInputsResult(
    std::vector<OrchardNote> inputs,
    uint64_t fee,
    uint64_t change)
    : inputs(inputs), fee(fee), change(change) {}
PickOrchardInputsResult::~PickOrchardInputsResult() = default;
PickOrchardInputsResult::PickOrchardInputsResult(
    const PickOrchardInputsResult& other) = default;
PickOrchardInputsResult::PickOrchardInputsResult(
    PickOrchardInputsResult&& other) = default;

#if BUILDFLAG(ENABLE_ORCHARD)
std::optional<PickOrchardInputsResult> PickZCashOrchardInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type) {
  if (notes.empty()) {
    return std::nullopt;
  }

  if (amount == kZCashFullAmount) {
    auto total_inputs_amount = CalculateInputsAmount(notes);

    base::CheckedNumeric<uint64_t> fee = CalculateZCashTxFee(
        0u, base::checked_cast<uint32_t>(notes.size()), output_type);

    if (!total_inputs_amount.IsValid() || !fee.IsValid()) {
      return std::nullopt;
    }
    // Check whether total_inputs_amount amount is not less than fee
    if (!base::CheckSub(total_inputs_amount, fee).IsValid()) {
      return std::nullopt;
    }
    return PickOrchardInputsResult{notes, fee.ValueOrDie(), 0};
  }

  std::vector<OrchardNote> mutable_notes = notes;

  std::ranges::sort(mutable_notes, [](auto& input1, auto& input2) {
    return input1.amount < input2.amount;
  });

  std::vector<OrchardNote> selected_inputs;
  for (auto& input : mutable_notes) {
    selected_inputs.push_back(input);
    auto total_inputs_amount = CalculateInputsAmount(selected_inputs);

    base::CheckedNumeric<uint64_t> fee = CalculateZCashTxFee(
        0u, base::checked_cast<uint32_t>(selected_inputs.size()), output_type);

    if (!total_inputs_amount.IsValid() || !fee.IsValid()) {
      return std::nullopt;
    }

    auto amount_and_fee = base::CheckAdd<uint64_t>(amount, fee);
    if (!amount_and_fee.IsValid()) {
      return std::nullopt;
    }

    auto change = base::CheckSub(total_inputs_amount, amount_and_fee);
    if (change.IsValid()) {
      return PickOrchardInputsResult{std::move(selected_inputs),
                                     fee.ValueOrDie(), change.ValueOrDie()};
    }
  }

  return std::nullopt;
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
