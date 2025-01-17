// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_DISCOVER_NEXT_UNUSED_ZCASH_ADDRESS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_DISCOVER_NEXT_UNUSED_ZCASH_ADDRESS_TASK_H_

#include <string>

#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ZCashDiscoverNextUnusedZCashAddressTask
    : public base::RefCounted<ZCashDiscoverNextUnusedZCashAddressTask> {
 public:
  ZCashDiscoverNextUnusedZCashAddressTask(
      base::PassKey<class ZCashWalletService> pass_key,
      base::WeakPtr<ZCashWalletService> zcash_wallet_service,
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashAddressPtr& start_address,
      ZCashWalletService::DiscoverNextUnusedAddressCallback callback);

  void Start();

 private:
  friend class base::RefCounted<ZCashDiscoverNextUnusedZCashAddressTask>;

  virtual ~ZCashDiscoverNextUnusedZCashAddressTask();

  void ScheduleWorkOnTask();

  mojom::ZCashAddressPtr GetNextAddress(const mojom::ZCashAddressPtr& address);

  void WorkOnTask();
  void OnGetIsKnownAddress(base::expected<bool, std::string> stats);
  void OnGetLastBlock(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  base::WeakPtr<ZCashWalletService> zcash_wallet_service_;
  mojom::AccountIdPtr account_id_;
  mojom::ZCashAddressPtr start_address_;
  mojom::ZCashAddressPtr current_address_;

  bool started_ = false;

  mojom::ZCashAddressPtr result_;
  std::optional<uint64_t> block_end_;
  std::optional<std::string> error_;
  ZCashWalletService::DiscoverNextUnusedAddressCallback callback_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_DISCOVER_NEXT_UNUSED_ZCASH_ADDRESS_TASK_H_
