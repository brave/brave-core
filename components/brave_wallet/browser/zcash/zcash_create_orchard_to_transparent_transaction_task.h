// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_TRANSPARENT_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_TRANSPARENT_TRANSACTION_TASK_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// This task takes Orchard notes from the provided account and creates a
// transaction which transfers funds to the provided transparent address.
class ZCashCreateOrchardToTransparentTransactionTask {
 public:
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  ZCashCreateOrchardToTransparentTransactionTask(
      std::variant<
          base::PassKey<
              class ZCashCreateOrchardToTransparentTransactionTaskTest>,
          base::PassKey<ZCashWalletService>> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      const std::string& transparent_address,
      uint64_t amount);
  ~ZCashCreateOrchardToTransparentTransactionTask();

  void Start(CreateTransactionCallback callback);

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetSpendableNotes();

  void OnGetSpendableNotes(
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error> result);

  void SetError(const std::string& error_string) { error_ = error_string; }

  void CreateTransaction();

  const raw_ref<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  ZCashActionContext context_;
  std::string transparent_address_;
  uint64_t amount_ = 0;

  std::optional<std::string> error_;

  std::optional<OrchardSyncState::SpendableNotesBundle> spendable_notes_;

  std::optional<ZCashTransaction> transaction_;

  ZCashWalletService::CreateTransactionCallback callback_;

  base::WeakPtrFactory<ZCashCreateOrchardToTransparentTransactionTask>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_TRANSPARENT_TRANSACTION_TASK_H_
