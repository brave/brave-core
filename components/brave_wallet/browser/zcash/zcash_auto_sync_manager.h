/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_AUTO_SYNC_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_AUTO_SYNC_MANAGER_H_

#include <string>

#include "base/timer/timer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"

namespace brave_wallet {
inline constexpr base::TimeDelta kZCashAutoSyncRefreshInterval =
    base::Minutes(5);
inline constexpr uint32_t kZCashAutoSyncMaxBlocksDelta =
    1152u * 7u;  // Equals to a week

class ZCashWalletService;

class ZCashAutoSyncManager {
 public:
  ZCashAutoSyncManager(ZCashWalletService& zcash_wallet_service,
                       ZCashActionContext action_context);
  ~ZCashAutoSyncManager();

  bool IsStarted();
  void Start();

 private:
  void OnTimerHit();
  void OnGetChainTipStatus(mojom::ZCashChainTipStatusPtr status,
                           const std::optional<std::string>& error);

  bool started_ = false;
  base::RepeatingTimer timer_;

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  ZCashActionContext zcash_action_context_;

  base::WeakPtrFactory<ZCashAutoSyncManager> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_AUTO_SYNC_MANAGER_H_
