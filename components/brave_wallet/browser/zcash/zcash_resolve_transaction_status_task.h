// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_TRANSACTION_STATUS_TASK_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_TRANSACTION_STATUS_TASK_H_

#include <memory>
#include <string>
#include <variant>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class ZCashTxMeta;

// Resolves transaction status task.
// Handles tx expiry cases.
class ZCashResolveTransactionStatusTask {
 public:
  using ZCashResolveTransactionStatusTaskCallback = base::OnceCallback<void(
      base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                     std::string>)>;

  ZCashResolveTransactionStatusTask(
      std::variant<base::PassKey<ZCashWalletService>,
                   base::PassKey<class ZCashResolveTransactionStatusTaskTest>>
          pass_key,
      ZCashActionContext context,
      ZCashWalletService& zcash_wallet_service,
      std::unique_ptr<ZCashTxMeta> tx_meta,
      ZCashResolveTransactionStatusTaskCallback callback);
  ~ZCashResolveTransactionStatusTask();

  void Start();

 private:
  void ScheduleWorkOnTask();
  void WorkOnTask();

  void GetChainTip();
  void OnGetChainTipResult(
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);

  void GetTransaction();
  void OnGetTransactionResult(
      base::expected<zcash::mojom::RawTransactionPtr, std::string> result);

  ZCashActionContext context_;
  raw_ref<ZCashWalletService> zcash_wallet_service_;
  std::unique_ptr<ZCashTxMeta> tx_meta_;
  ZCashResolveTransactionStatusTaskCallback callback_;

  bool started_ = false;

  std::optional<std::string> error_;
  std::optional<zcash::mojom::BlockIDPtr> chain_tip_;
  std::optional<zcash::mojom::RawTransactionPtr> transaction_;

  base::WeakPtrFactory<ZCashResolveTransactionStatusTask> weak_ptr_factory_{
      this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RESOLVE_TRANSACTION_STATUS_TASK_H_
