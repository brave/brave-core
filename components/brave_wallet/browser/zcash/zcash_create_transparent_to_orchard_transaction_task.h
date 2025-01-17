// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TO_ORCHARD_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TO_ORCHARD_TRANSACTION_TASK_H_

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// This tasks takes all transparent UTXOs for the provided account and
// creates transaction which transfers this funds to the provided shielded
// address.
class ZCashCreateTransparentToOrchardTransactionTask {
 public:
  ZCashCreateTransparentToOrchardTransactionTask(
      absl::variant<
          base::PassKey<
              class ZCashCreateTransparentToOrchardTransactionTaskTest>,
          base::PassKey<ZCashWalletService>> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      const OrchardAddrRawPart& receiver,
      std::optional<OrchardMemo> memo,
      uint64_t amount,
      ZCashWalletService::CreateTransactionCallback callback);
  ~ZCashCreateTransparentToOrchardTransactionTask();

  void Start();

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetAllUtxos();
  void GetChangeAddress();

  void OnGetChangeAddress(
      base::expected<mojom::ZCashAddressPtr, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);

  void SetError(const std::string& error_string) { error_ = error_string; }

  void CreateTransaction();

  const raw_ref<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  ZCashActionContext context_;
  OrchardAddrRawPart receiver_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_;

  bool started_ = false;

  std::optional<std::string> error_;

  std::optional<ZCashWalletService::UtxoMap> utxo_map_;
  mojom::ZCashAddressPtr change_address_;

  std::optional<ZCashTransaction> transaction_;

  ZCashWalletService::CreateTransactionCallback callback_;

  base::WeakPtrFactory<ZCashCreateTransparentToOrchardTransactionTask>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TO_ORCHARD_TRANSACTION_TASK_H_
