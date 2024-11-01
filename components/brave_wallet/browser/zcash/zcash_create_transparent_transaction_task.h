// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TRANSACTION_TASK_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ZCashCreateTransparentTransactionTask {
 public:
  using UtxoMap = ZCashWalletService::UtxoMap;
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  virtual ~ZCashCreateTransparentTransactionTask();

  void ScheduleWorkOnTask();

 private:
  friend class ZCashWalletService;

  ZCashCreateTransparentTransactionTask(
      ZCashWalletService& zcash_wallet_service,
      const std::string& chain_id,
      const mojom::AccountIdPtr& account_id,
      const std::string& address_to,
      uint64_t amount,
      CreateTransactionCallback callback);

  void WorkOnTask();

  bool IsTestnet() { return chain_id_ == mojom::kZCashTestnet; }

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool PrepareOutputs();

  void OnGetChainHeight(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);
  void OnGetChangeAddress(
      base::expected<mojom::ZCashAddressPtr, std::string> result);

  const raw_ref<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  uint64_t amount_;
  CreateTransactionCallback callback_;

  std::optional<uint32_t> chain_height_;
  ZCashWalletService::UtxoMap utxo_map_;

  std::optional<std::string> error_;
  ZCashTransaction transaction_;

  mojom::ZCashAddressPtr change_address_;

  base::WeakPtrFactory<ZCashCreateTransparentTransactionTask> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TRANSACTION_TASK_H_
