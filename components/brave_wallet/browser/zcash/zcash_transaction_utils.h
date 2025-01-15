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

std::optional<PickInputsResult> PickZCashTransparentInputs(
    const ZCashWalletService::UtxoMap& utxo_map,
    uint64_t amount,
    size_t orchard_actions_count);

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

#if BUILDFLAG(ENABLE_ORCHARD)
std::optional<PickOrchardInputsResult> PickZCashOrchardInputs(
    const std::vector<OrchardNote>& notes,
    uint64_t amount);
#endif  // BUILDFLAG(ENABLE_ORCHARD)

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_UTILS_H_
