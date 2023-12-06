/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_TASKS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_TASKS_H_

#include <set>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class GetTransparentUtxosContext
    : public base::RefCountedThreadSafe<GetTransparentUtxosContext> {
 public:
  GetTransparentUtxosContext();
  using GetUtxosCallback = ZCashWalletService::GetUtxosCallback;

  std::set<std::string> addresses;
  ZCashWalletService::UtxoMap utxos;
  std::optional<std::string> error;
  GetUtxosCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetTransparentUtxosContext>;

  virtual ~GetTransparentUtxosContext();
};

class DiscoverNextUnusedZCashAddressTask
    : public base::RefCounted<DiscoverNextUnusedZCashAddressTask> {
 public:
  DiscoverNextUnusedZCashAddressTask(
      base::WeakPtr<ZCashWalletService> zcash_wallet_service,
      mojom::AccountIdPtr account_id,
      mojom::ZCashAddressPtr start_address,
      ZCashWalletService::DiscoverNextUnusedAddressCallback callback);
  void ScheduleWorkOnTask();

 private:
  friend class base::RefCounted<DiscoverNextUnusedZCashAddressTask>;
  virtual ~DiscoverNextUnusedZCashAddressTask();

  mojom::ZCashAddressPtr GetNextAddress(const mojom::ZCashAddressPtr& address);

  void WorkOnTask();
  void OnGetIsKnownAddress(base::expected<bool, std::string> stats);
  void OnGetLastBlock(base::expected<zcash::BlockID, std::string> result);

  base::WeakPtr<ZCashWalletService> zcash_wallet_service_;
  mojom::AccountIdPtr account_id_;
  mojom::ZCashAddressPtr start_address_;
  mojom::ZCashAddressPtr current_address_;
  mojom::ZCashAddressPtr result_;
  std::optional<uint64_t> block_end_;
  std::optional<std::string> error_;
  ZCashWalletService::DiscoverNextUnusedAddressCallback callback_;
};

class CreateTransparentTransactionTask {
 public:
  using UtxoMap = ZCashWalletService::UtxoMap;
  using CreateTransactionCallback =
      ZCashWalletService::CreateTransactionCallback;

  CreateTransparentTransactionTask(ZCashWalletService* zcash_wallet_service,
                                   const std::string& chain_id,
                                   const mojom::AccountIdPtr& account_id,
                                   const std::string& address_to,
                                   uint64_t amount,
                                   CreateTransactionCallback callback);
  virtual ~CreateTransparentTransactionTask();

  void ScheduleWorkOnTask();

 private:
  bool IsTestnet() { return chain_id_ == mojom::kZCashTestnet; }
  void WorkOnTask();

  void SetError(const std::string& error_string) { error_ = error_string; }

  bool PickInputs();
  bool PrepareOutputs();

  void OnGetChainHeight(base::expected<zcash::BlockID, std::string> result);
  void OnGetUtxos(
      base::expected<ZCashWalletService::UtxoMap, std::string> utxo_map);
  void OnGetChangeAddress(
      base::expected<mojom::ZCashAddressPtr, std::string> result);

  raw_ptr<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  CreateTransactionCallback callback_;

  std::optional<uint32_t> chain_height_;
  ZCashWalletService::UtxoMap utxo_map_;

  std::optional<std::string> error_;
  ZCashTransaction transaction_;

  mojom::ZCashAddressPtr change_address_;

  base::WeakPtrFactory<CreateTransparentTransactionTask> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_TASKS_H_
