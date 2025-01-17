/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

#include <utility>

namespace brave_wallet {

namespace {

uint64_t CalculateInputsAmount(
    const std::vector<ZCashTransaction::TxInput>& inputs) {
  uint64_t total_value = 0;
  for (const auto& input : inputs) {
    total_value += input.utxo_value;
  }
  return total_value;
}

#if BUILDFLAG(ENABLE_ORCHARD)

uint64_t CalculateInputsAmount(const std::vector<OrchardNote>& notes) {
  uint64_t total_value = 0;
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
    size_t orchard_actions_count) {
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
    auto fee = CalculateZCashTxFee(all_inputs.size(), orchard_actions_count);
    if (CalculateInputsAmount(all_inputs) < fee) {
      return std::nullopt;
    }
    return PickInputsResult{std::move(all_inputs), fee, 0};
  }

  base::ranges::sort(all_inputs, [](auto& input1, auto& input2) {
    return input1.utxo_value < input2.utxo_value;
  });

  std::vector<ZCashTransaction::TxInput> selected_inputs;
  uint64_t fee = 0;
  for (auto& input : all_inputs) {
    selected_inputs.push_back(std::move(input));
    fee = CalculateZCashTxFee(selected_inputs.size(), orchard_actions_count);

    auto total_inputs_amount = CalculateInputsAmount(selected_inputs);
    if (total_inputs_amount >= amount + fee) {
      return PickInputsResult{std::move(selected_inputs), fee,
                              total_inputs_amount - amount - fee};
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
    uint64_t amount) {
  if (amount == kZCashFullAmount) {
    auto fee =
        CalculateZCashTxFee(0, notes.size() + 1 /* orchard actions count */);
    if (CalculateInputsAmount(notes) < fee) {
      return std::nullopt;
    }
    return PickOrchardInputsResult{notes, fee, 0};
  }

  std::vector<OrchardNote> mutable_notes = notes;

  base::ranges::sort(mutable_notes, [](auto& input1, auto& input2) {
    return input1.amount < input2.amount;
  });

  std::vector<OrchardNote> selected_inputs;
  uint64_t fee = 0;
  for (auto& input : mutable_notes) {
    selected_inputs.push_back(input);
    fee = CalculateZCashTxFee(0, selected_inputs.size() + 1);

    auto total_inputs_amount = CalculateInputsAmount(selected_inputs);
    if (total_inputs_amount >= amount + fee) {
      return PickOrchardInputsResult{std::move(selected_inputs), fee,
                                     total_inputs_amount - amount - fee};
    }
  }

  return std::nullopt;
}
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet
