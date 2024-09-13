// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELD_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELD_TRANSACTION_TASK_H_

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

// This tasks takes all transparent UTXOs for an account and
// creates transaction which transfers this funds to the internal shielded
// address.
class ZCashCreateShieldTransactionTask {
 public:
  ~ZCashCreateShieldTransactionTask();

  void ScheduleWorkOnTask();

 private:
  friend class ZCashWalletService;

  ZCashCreateShieldTransactionTask(
      ZCashWalletService* zcash_wallet_service,
      const std::string& chain_id,
      const mojom::AccountIdPtr& account_id,
      const OrchardAddrRawPart& receiver,
      std::optional<OrchardMemo> memo,
      uint64_t amount,
      ZCashWalletService::CreateTransactionCallback callback);

  void WorkOnTask();

  void GetAllUtxos();
  void GetTreeState();
  void GetChainHeight();
  void GetChangeAddress();

  void OnGetChangeAddress(
      base::expected<mojom::ZCashAddressPtr, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool CreateTransaction();

  raw_ptr<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  OrchardAddrRawPart receiver_;
  std::optional<OrchardMemo> memo_;
  uint64_t amount_;

  std::optional<std::string> error_;

  std::optional<ZCashWalletService::UtxoMap> utxo_map_;
  mojom::ZCashAddressPtr change_address_;

  std::optional<ZCashTransaction> transaction_;

  ZCashWalletService::CreateTransactionCallback callback_;

  base::WeakPtrFactory<ZCashCreateShieldTransactionTask> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_SHIELD_TRANSACTION_TASK_H_
