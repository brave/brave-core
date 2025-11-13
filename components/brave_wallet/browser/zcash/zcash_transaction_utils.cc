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

}  // namespace

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
    base::CheckedNumeric<uint64_t> fee = 0u;
    switch (output_type) {
      case ZCashTargetOutputType::kTransparent:
        fee = CalculateZCashTxFee(all_inputs.size(), 1u, 0u, 0u);
        break;
      case ZCashTargetOutputType::kOrchard:
        fee = CalculateZCashTxFee(all_inputs.size(), 0u, 0u, 1u);
        break;
      default:
        NOTREACHED();
    }

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

    base::CheckedNumeric<uint64_t> fee = 0;
    switch (output_type) {
      case ZCashTargetOutputType::kTransparent:
        fee = CalculateZCashTxFee(selected_inputs.size(), 2u, 0u, 0u);
        break;
      case ZCashTargetOutputType::kOrchard:
        fee = CalculateZCashTxFee(selected_inputs.size(), 1u, 0u, 1u);
        break;
      default:
        NOTREACHED();
    }

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
  if (amount == kZCashFullAmount) {
    auto total_inputs_amount = CalculateInputsAmount(notes);

    base::CheckedNumeric<uint64_t> fee = 0;
    switch (output_type) {
      case ZCashTargetOutputType::kTransparent:
        fee = CalculateZCashTxFee(0u, 1u, notes.size(), 0u);
        break;
      case ZCashTargetOutputType::kOrchard:
        fee = CalculateZCashTxFee(0u, 0u, notes.size(), 1u);
        break;
      default:
        NOTREACHED();
    }

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
    base::CheckedNumeric<uint64_t> fee = 0u;
    switch (output_type) {
      case ZCashTargetOutputType::kTransparent:
        fee = CalculateZCashTxFee(0u, 1u, selected_inputs.size(), 1u);
        break;
      case ZCashTargetOutputType::kOrchard:
        fee = CalculateZCashTxFee(0u, 0u, selected_inputs.size(), 2u);
        break;
      default:
        NOTREACHED();
    }

    if (!total_inputs_amount.IsValid() || !fee.IsValid()) {
      return std::nullopt;
    }

    auto amount_and_fee = base::CheckAdd<uint64_t>(amount, fee);
    if (!amount_and_fee.IsValid()) {
      return std::nullopt;
    }

    auto change = base::CheckSub(total_inputs_amount, amount_and_fee);
    if (change.IsValid()) {
      LOG(ERROR) << "XXXZZZ PickZCashOrchardInputs - Selected "
                 << selected_inputs.size() << " inputs, total: "
                 << static_cast<uint64_t>(total_inputs_amount.ValueOrDie())
                 << ", amount: " << amount
                 << ", fee: " << static_cast<uint64_t>(fee.ValueOrDie())
                 << ", change: " << static_cast<uint64_t>(change.ValueOrDie());
      return PickOrchardInputsResult{std::move(selected_inputs),
                                     fee.ValueOrDie(), change.ValueOrDie()};
    }
  }

  return std::nullopt;
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
