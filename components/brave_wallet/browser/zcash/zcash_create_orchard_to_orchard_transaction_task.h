// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_ORCHARD_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_ORCHARD_TRANSACTION_TASK_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

// Creates transaction within Orchard pool.
// Uses shielded inputs and shielded output.
class ZCashCreateOrchardToOrchardTransactionTask {
 public:
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  ZCashCreateOrchardToOrchardTransactionTask(
      absl::variant<
          base::PassKey<class ZCashCreateOrchardToOrchardTransactionTaskTest>,
          base::PassKey<ZCashWalletService>> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      const OrchardAddrRawPart& receiver,
      std::optional<OrchardMemo> memo,
      uint64_t amount,
      ZCashWalletService::CreateTransactionCallback callback);

  virtual ~ZCashCreateOrchardToOrchardTransactionTask();

  void Start();

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetSpendableNotes();
  void OnGetSpendableNotes(
      base::expected<std::vector<OrchardNote>, OrchardStorage::Error> result);
  void CreateTransaction();

  void GetAnchor();
  void OnGetAnchor();

  void CalculateWitness();
  void OnWittnessCalculated();

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  ZCashActionContext context_;
  OrchardAddrRawPart receiver_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_;
  ZCashWalletService::CreateTransactionCallback callback_;

  bool started_ = false;

  std::optional<std::string> error_;
  std::optional<std::vector<OrchardNote>> spendable_notes_;
  std::optional<PickOrchardInputsResult> picked_notes_;
  std::optional<OrchardSpendsBundle> spends_bundle_;
  std::optional<ZCashTransaction> transaction_;

  base::WeakPtrFactory<ZCashCreateOrchardToOrchardTransactionTask>
      weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_ORCHARD_TO_ORCHARD_TRANSACTION_TASK_H_
