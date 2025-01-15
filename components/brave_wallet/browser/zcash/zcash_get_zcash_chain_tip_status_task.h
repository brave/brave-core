// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GET_ZCASH_CHAIN_TIP_STATUS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GET_ZCASH_CHAIN_TIP_STATUS_TASK_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

// Resolves information regarding current chain tip and the latest scanned
// block.
class ZCashGetZCashChainTipStatusTask {
 public:
  using ZCashGetZCashChainTipStatusTaskCallback = base::OnceCallback<void(
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>)>;

  ZCashGetZCashChainTipStatusTask(
      base::PassKey<class ZCashWalletService> pass_key,
      ZCashWalletService& zcash_wallet_service,
      ZCashActionContext context,
      ZCashGetZCashChainTipStatusTaskCallback callback);
  ~ZCashGetZCashChainTipStatusTask();

  void Start();

 private:
  void WorkOnTask();
  void ScheduleWorkOnTask();

  void GetAccountMeta();
  void OnGetAccountMeta(
      base::expected<std::optional<OrchardStorage::AccountMeta>,
                     OrchardStorage::Error>);

  void GetChainTipHeight();
  void OnGetChainTipHeightResult(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  ZCashActionContext context_;
  ZCashGetZCashChainTipStatusTaskCallback callback_;

  std::optional<OrchardStorage::AccountMeta> account_meta_;
  std::optional<uint32_t> chain_tip_height_;
  std::optional<std::string> error_;

  bool started_ = false;

  base::WeakPtrFactory<ZCashGetZCashChainTipStatusTask> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_GET_ZCASH_CHAIN_TIP_STATUS_TASK_H_
