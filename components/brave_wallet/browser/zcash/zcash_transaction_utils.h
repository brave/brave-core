/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_UTILS_H_

#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

enum class ZCashTargetOutputType {
  kTransparent,
  kOrchard,
  kIronwood,
};

struct PickInputsResult {
  std::vector<ZCashTransaction::TxInput> inputs;
  uint64_t fee;
  uint64_t change;

  PickInputsResult(std::vector<ZCashTransaction::TxInput> inputs,
                   uint64_t fee,
                   uint64_t change);
  ~PickInputsResult();
  PickInputsResult(const PickInputsResult& other);
  PickInputsResult& operator=(const PickInputsResult& other) = delete;
  PickInputsResult(PickInputsResult&& other);
  PickInputsResult& operator=(PickInputsResult&& other) = delete;
};

// Exactly one of the input counts must be non-zero — mixed inputs are not
// supported. Change, when `has_change` is true, is assumed to go back to the
// pool the inputs came from; pass false for full amount sends, which have no
// change output.
// The legacy Orchard pool is only spent inside a v6 transaction (post-NU6.3),
// where its actions can't pair a spend with an unrelated output, so its bundle
// needs `spends + outputs` actions rather than `max(spends, outputs)`.
base::CheckedNumeric<uint64_t> CalculateZCashTxFee(
    const base::StrictNumeric<uint32_t> transparent_input_count,
    const base::StrictNumeric<uint32_t> orchard_input_count,
    const base::StrictNumeric<uint32_t> ironwood_input_count,
    ZCashTargetOutputType output_type,
    bool has_change);

std::optional<PickInputsResult> PickZCashTransparentInputs(
    const ZCashWalletService::UtxoMap& utxo_map,
    uint64_t amount,
    ZCashTargetOutputType output_type);

struct PickOrchardInputsResult {
  std::vector<OrchardNote> inputs;
  uint64_t fee;
  uint64_t change;

  PickOrchardInputsResult(std::vector<OrchardNote> inputs,
                          uint64_t fee,
                          uint64_t change);
  ~PickOrchardInputsResult();
  PickOrchardInputsResult(const PickOrchardInputsResult& other);
  PickOrchardInputsResult& operator=(const PickOrchardInputsResult& other) =
      delete;
  PickOrchardInputsResult(PickOrchardInputsResult&& other);
  PickOrchardInputsResult& operator=(PickOrchardInputsResult&& other) = delete;
};

// Picks notes from the legacy Orchard pool.
std::optional<PickOrchardInputsResult> PickZCashOrchardInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type);

// Picks notes from the Ironwood pool. Ironwood notes are `OrchardNote`s too —
// only the pool they belong to differs (see `OrchardPool`). Unlike the legacy
// Orchard pool, Ironwood always permits cross-address transfers, so its
// actions pair a spend with an output and its fee is correspondingly lower.
std::optional<PickOrchardInputsResult> PickZCashIronwoodInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_UTILS_H_
