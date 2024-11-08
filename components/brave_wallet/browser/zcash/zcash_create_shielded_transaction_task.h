// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELDED_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELDED_TRANSACTION_TASK_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_transaction_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

class ZCashCreateShieldedTransactionTask {
 public:
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  ZCashCreateShieldedTransactionTask(
      ZCashWalletService* zcash_wallet_service,
      const std::string& chain_id,
      const mojom::AccountIdPtr& account_id,
      const OrchardAddrRawPart& receiver,
      std::optional<OrchardMemo> memo,
      uint64_t amount,
      ZCashWalletService::CreateTransactionCallback callback);

  virtual ~ZCashCreateShieldedTransactionTask();

  void ScheduleWorkOnTask();

 private:
  void WorkOnTask();

  void GetSpendableNotes();
  void OnGetSpendableNotes(
      base::expected<std::vector<OrchardNote>, OrchardStorage::Error> result);
  void CreateTransaction();

  void PickOrchardInputs();

  void GetAnchor();
  void OnGetAnchor();

  void CalculateWitness();
  void OnWittnessCalculated();

  raw_ptr<ZCashWalletService> zcash_wallet_service_;
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  OrchardAddrRawPart receiver_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_;
  ZCashWalletService::CreateTransactionCallback callback_;

  std::optional<std::string> error_;
  std::optional<std::vector<OrchardNote>> spendable_notes_;
  std::optional<PickOrchardInputsResult> picked_notes_;
  std::optional<OrchardSpendsBundle> spends_bundle_;
  std::optional<ZCashTransaction> transaction_;

  base::WeakPtrFactory<ZCashCreateShieldedTransactionTask> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELDED_TRANSACTION_TASK_H_
