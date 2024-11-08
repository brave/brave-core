/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_BALANCE_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_BALANCE_TASK_H_

#include <optional>

#include "base/memory/raw_ref.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"

namespace brave_wallet {

class ZCashResolveBalanceTask {
 public:
  using ZCashResolveBalanceTaskCallback = base::OnceCallback<void(
      base::expected<mojom::ZCashBalancePtr, std::string>)>;
  ZCashResolveBalanceTask(ZCashWalletService& zcash_wallet_service,
                          const std::string& chain_id,
                          mojom::AccountIdPtr account_id,
                          ZCashResolveBalanceTaskCallback callback);
  ~ZCashResolveBalanceTask();

  void ScheduleWorkOnTask();

 private:
  void WorkOnTask();

  void OnDiscoveryDoneForBalance(
      ZCashWalletService::RunDiscoveryResult discovery_result);

  void OnUtxosResolvedForBalance(
      base::expected<ZCashWalletService::UtxoMap, std::string> result);

#if BUILDFLAG(ENABLE_ORCHARD)
  void OnGetSpendableNotes(base::expected<std::vector<OrchardNote>,
                                          ZCashOrchardStorage::Error> result);

#endif  // BUILDFLAG(ENABLE_ORCHARD)

  void CreateBalance();

  const raw_ref<ZCashWalletService> zcash_wallet_service_;  // Owns this
  std::string chain_id_;
  mojom::AccountIdPtr account_id_;
  ZCashResolveBalanceTaskCallback callback_;

  std::optional<std::string> error_;
  std::optional<ZCashWalletService::RunDiscoveryResult> discovery_result_;
  std::optional<ZCashWalletService::UtxoMap> utxo_map_;
  std::optional<mojom::ZCashBalancePtr> result_;

#if BUILDFLAG(ENABLE_ORCHARD)
  std::optional<std::vector<OrchardNote>> orchard_notes_;
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  base::WeakPtrFactory<ZCashResolveBalanceTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_BALANCE_TASK_H_
