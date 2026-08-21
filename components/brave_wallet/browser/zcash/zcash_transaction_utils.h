/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

// `orchard_cross_address_disabled` must be true when `orchard_input_notes`
// spends the legacy Orchard pool inside a v6 transaction (post-NU6.3), since
// that pool's actions can't pair a spend with an unrelated output — it changes
// how many Orchard actions the resulting bundle needs, and therefore the fee.
base::CheckedNumeric<uint64_t> CalculateZCashTxFee(
    const base::StrictNumeric<uint32_t> transparent_input_count,
    const base::StrictNumeric<uint32_t> orchard_input_notes,
    ZCashTargetOutputType output_type,
    bool orchard_cross_address_disabled = false);

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

// See `CalculateZCashTxFee` for `orchard_cross_address_disabled`.
std::optional<PickOrchardInputsResult> PickZCashOrchardInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount,
    ZCashTargetOutputType output_type,
    bool orchard_cross_address_disabled);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_UTILS_H_
