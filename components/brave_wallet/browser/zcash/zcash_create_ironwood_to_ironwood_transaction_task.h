// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_IRONWOOD_TO_IRONWOOD_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_IRONWOOD_TO_IRONWOOD_TRANSACTION_TASK_H_

#include <string>
#include <variant>

#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// Creates a single-pool v6 transaction: spends Ironwood notes, outputs to
// the Ironwood pool.
class ZCashCreateIronwoodToIronwoodTransactionTask {
 public:
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  ZCashCreateIronwoodToIronwoodTransactionTask(
      std::variant<
          base::PassKey<class ZCashCreateIronwoodToIronwoodTransactionTaskTest>,
          base::PassKey<ZCashWalletService>> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      const OrchardAddrRawPart& receiver,
      std::optional<OrchardMemo> memo,
      uint64_t amount);

  virtual ~ZCashCreateIronwoodToIronwoodTransactionTask();

  void Start(CreateTransactionCallback callback);

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetSpendableNotes();
  void OnGetSpendableNotes(
      base::expected<std::optional<OrchardSyncState::SpendableNotesBundle>,
                     OrchardStorage::Error> result);
  void CreateTransaction();

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  ZCashActionContext context_;
  OrchardAddrRawPart receiver_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_;
  ZCashWalletService::CreateTransactionCallback callback_;

  std::optional<std::string> error_;
  std::optional<OrchardSyncState::SpendableNotesBundle> spendable_notes_;
  std::optional<ZCashTransaction> transaction_;

  base::WeakPtrFactory<ZCashCreateIronwoodToIronwoodTransactionTask>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_IRONWOOD_TO_IRONWOOD_TRANSACTION_TASK_H_
