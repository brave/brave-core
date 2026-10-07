/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"

#include <algorithm>
#include <iterator>
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

base::CheckedNumeric<uint64_t> CalculateInputsAmount(
    const std::vector<OrchardNote>& notes) {
  base::CheckedNumeric<uint64_t> total_value = 0u;
  for (const auto& note : notes) {
    total_value += note.amount;
  }
  return total_value;
}

// https://github.com/zcash/orchard/blob/9d89b504c52dc69064ca431e8311a4cd1c279b44/src/builder.rs#L120-L148
// The legacy Orchard pool is only ever spent inside a v6 transaction
// (post-NU6.3), where cross-address transfers are mandatorily disabled: a
// spend and an output never share an action — each is padded with a fabricated
// zero-valued counterpart — so the actual action count is `spends + outputs`,
// not `max(spends, outputs)`. Undercounting here underpays the ZIP-317 fee and
// gets the tx rejected as "unpaid actions".
base::CheckedNumeric<uint32_t> GetLegacyOrchardActionsCount(
    const base::StrictNumeric<uint32_t> input_notes,
    const base::StrictNumeric<uint32_t> output_notes) {
  if (input_notes == 0u && output_notes == 0u) {
    return 0u;
  }

  base::CheckedNumeric<uint32_t> requested_actions =
      base::CheckAdd<uint32_t>(input_notes, output_notes);
  if (!requested_actions.IsValid()) {
    return requested_actions;
  }
  return base::CheckMax<uint32_t>(requested_actions.ValueOrDie(),
                                  kMinOrchardActionsCountForFee);
}

// The Ironwood pool permits cross-address transfers under every protocol
// version - only the legacy Orchard pool post-NU6.3 mandates the restriction.
// See `BundleVersion::permits_cross_address_transfers` in
// third_party/rust/chromium_crates_io/vendor/orchard-v0_15/src/bundle.rs.
base::CheckedNumeric<uint32_t> GetIronwoodActionsCount(
    const base::StrictNumeric<uint32_t> input_notes,
    const base::StrictNumeric<uint32_t> output_notes) {
  if (input_notes == 0u && output_notes == 0u) {
    return 0u;
  }

  return base::CheckMax<uint32_t>(input_notes, output_notes,
                                  kMinOrchardActionsCountForFee);
}

}  // namespace

// https://zips.z.cash/zip-0317
// The transparent, legacy Orchard and Ironwood bundles each count their own
// actions, and change goes back to the pool the inputs came from. The Ironwood
// bundle pairs a spend with an output in one action, so it needs
// `max(spends, outputs)`; the legacy Orchard bundle cannot (see
// `GetLegacyOrchardActionsCount`) and needs `spends + outputs`. With `i` for
// inputs and `c` for the change output (1 when `has_change`, otherwise 0):
// t->t:
// fee = max(2, max(i, 1 + c)) * 5000
// t->ironwood:
// fee = max(2, max(i, c) + max(0, 1, 2)) * 5000
// ironwood->t:
// fee = max(2, max(0, 1) + max(i, c, 2)) * 5000
// ironwood->ironwood:
// fee = max(2, max(i, 1 + c, 2)) * 5000
// orchard->t:
// fee = max(2, max(0, 1) + max(i + c, 2)) * 5000
// orchard->ironwood:
// fee = max(2, max(i + c, 2) + max(0, 1, 2)) * 5000.
base::CheckedNumeric<uint64_t> CalculateZCashTxFee(
    const base::StrictNumeric<uint32_t> transparent_input_count,
    const base::StrictNumeric<uint32_t> orchard_input_count,
    const base::StrictNumeric<uint32_t> ironwood_input_count,
    ZCashTargetOutputType output_type,
    bool has_change) {
  // Mixed inputs are not supported, so inputs come from exactly one pool.
  CHECK_EQ(1, (transparent_input_count != 0u) + (orchard_input_count != 0u) +
                  (ironwood_input_count != 0u));

  // Basic outputs setup - add a change output to the pool being spent from.
  base::CheckedNumeric<uint32_t> transparent_output_count =
      has_change && transparent_input_count != 0u ? 1u : 0u;
  base::CheckedNumeric<uint32_t> orchard_output_count =
      has_change && orchard_input_count != 0u ? 1u : 0u;
  base::CheckedNumeric<uint32_t> ironwood_output_count =
      has_change && ironwood_input_count != 0u ? 1u : 0u;

  // Add a target output.
  switch (output_type) {
    case ZCashTargetOutputType::kTransparent:
      transparent_output_count++;
      break;
    case ZCashTargetOutputType::kOrchard:
      orchard_output_count++;
      break;
    case ZCashTargetOutputType::kIronwood:
      ironwood_output_count++;
      break;
    default:
      NOTREACHED();
  }

  base::CheckedNumeric<uint32_t> orchard_actions_count =
      GetLegacyOrchardActionsCount(orchard_input_count,
                                   orchard_output_count.ValueOrDie());
  base::CheckedNumeric<uint32_t> ironwood_actions_count =
      GetIronwoodActionsCount(ironwood_input_count,
                              ironwood_output_count.ValueOrDie());
  // https://github.com/zcash/librustzcash/blob/8eb78dfae38ca1c91a108a86a4a3b5505766c3f6/zcash_primitives/src/transaction/fees/zip317.rs#L188
  base::CheckedNumeric<uint32_t> logical_actions_count =
      base::CheckMax<uint32_t>(transparent_input_count,
                               transparent_output_count) +
      orchard_actions_count + ironwood_actions_count;
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
    base::CheckedNumeric<uint64_t> fee =
        CalculateZCashTxFee(base::checked_cast<uint32_t>(all_inputs.size()), 0u,
                            0u, output_type, /*has_change=*/false);
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

    // The change amount depends on the fee, so assume a change output here.
    // That may overestimate the fee when the picked inputs happen to leave no
    // change, which is safe - underpaying gets the tx rejected.
    base::CheckedNumeric<uint64_t> fee = CalculateZCashTxFee(
        base::checked_cast<uint32_t>(selected_inputs.size()), 0u, 0u,
        output_type, /*has_change=*/true);

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

namespace {

// Shared implementation of the legacy Orchard and Ironwood pickers. Both pools
// hold `OrchardNote`s and are picked from the same way; `pool` only decides
// which bundle the notes are billed to when computing the fee.
std::optional<PickOrchardInputsResult> PickShieldedInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type,
    OrchardPool pool) {
  auto calculate_fee = [&](size_t input_count, bool has_change) {
    auto count = base::checked_cast<uint32_t>(input_count);
    return CalculateZCashTxFee(0u, pool == OrchardPool::kOrchard ? count : 0u,
                               pool == OrchardPool::kIronwood ? count : 0u,
                               output_type, has_change);
  };

  if (notes.empty()) {
    return std::nullopt;
  }

  if (amount == kZCashFullAmount) {
    auto total_inputs_amount = CalculateInputsAmount(notes);

    // Full amount case - no change output.
    base::CheckedNumeric<uint64_t> fee =
        calculate_fee(notes.size(), /*has_change=*/false);

    if (!total_inputs_amount.IsValid() || !fee.IsValid()) {
      return std::nullopt;
    }
    // Check whether total_inputs_amount amount is not less than fee
    if (!base::CheckSub(total_inputs_amount, fee).IsValid()) {
      return std::nullopt;
    }
    return PickOrchardInputsResult{notes, fee.ValueOrDie(), 0};
  }

  std::vector<OrchardNote> mutable_notes;
  std::ranges::copy_if(notes, std::back_inserter(mutable_notes),
                       [](auto& note) { return note.amount != 0; });

  std::ranges::sort(mutable_notes, [](auto& input1, auto& input2) {
    return input1.amount < input2.amount;
  });

  std::vector<OrchardNote> selected_inputs;
  for (auto& input : mutable_notes) {
    selected_inputs.push_back(input);
    auto total_inputs_amount = CalculateInputsAmount(selected_inputs);

    // The change amount depends on the fee, so assume a change output here.
    // That may overestimate the fee when the picked notes happen to leave no
    // change, which is safe - underpaying gets the tx rejected.
    base::CheckedNumeric<uint64_t> fee =
        calculate_fee(selected_inputs.size(), /*has_change=*/true);

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

}  // namespace

std::optional<PickOrchardInputsResult> PickZCashOrchardInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type) {
  return PickShieldedInputs(notes, amount, output_type, OrchardPool::kOrchard);
}

std::optional<PickOrchardInputsResult> PickZCashIronwoodInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type) {
  return PickShieldedInputs(notes, amount, output_type, OrchardPool::kIronwood);
}

}  // namespace brave_wallet
