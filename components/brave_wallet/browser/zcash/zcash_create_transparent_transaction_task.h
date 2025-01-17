// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TRANSACTION_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_CREATE_TRANSPARENT_TRANSACTION_TASK_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

// Creates transaction with transparent inputs and transparent outputs.
class ZCashCreateTransparentTransactionTask {
 public:
  using UtxoMap = ZCashWalletService::UtxoMap;
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;
  ZCashCreateTransparentTransactionTask(
      base::PassKey<class ZCashWalletService> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      const std::string& address_to,
      uint64_t amount,
      CreateTransactionCallback callback);
  virtual ~ZCashCreateTransparentTransactionTask();

  void Start();

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  bool IsTestnet() { return context_.chain_id == mojom::kZCashTestnet; }

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool PrepareOutputs();

  void OnGetChainHeight(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);
  void OnGetChangeAddress(
      base::expected<mojom::ZCashAddressPtr, std::string> result);

  const raw_ref<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  ZCashActionContext context_;
  uint64_t amount_;
  CreateTransactionCallback callback_;

  bool started_ = false;

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
