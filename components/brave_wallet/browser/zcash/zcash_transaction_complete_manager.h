// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_COMPLETE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_COMPLETE_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/internal/orchard_bundle_manager.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/buildflags.h"

namespace brave_wallet {

class ZCashWalletService;

// Completes transaction by signing transparent inputs and generating orchard
// part(if needed)
class ZCashTransactionCompleteManager {
 public:
  using CompleteTransactionCallback =
      base::OnceCallback<void(base::expected<ZCashTransaction, std::string>)>;
  explicit ZCashTransactionCompleteManager(
      ZCashWalletService* zcash_wallet_service);
  ~ZCashTransactionCompleteManager();
  void CompleteTransaction(const std::string& chain_id,
                           const ZCashTransaction& transaction,
                           const mojom::AccountIdPtr& account_id,
                           CompleteTransactionCallback callback);

 private:
  struct ParamsBundle {
    std::string chain_id;
    ZCashTransaction transaction;
    mojom::AccountIdPtr account_id;
    CompleteTransactionCallback callback;
    ParamsBundle(std::string chain_id,
                 ZCashTransaction transaction,
                 mojom::AccountIdPtr account_id,
                 CompleteTransactionCallback callback);
    ~ParamsBundle();
    ParamsBundle(ParamsBundle& other) = delete;
    ParamsBundle(ParamsBundle&& other);
  };

  void SignTransparentPart(ParamsBundle params);

  void OnGetLatestBlockHeight(
      ParamsBundle params,
      base::expected<zcash::mojom::BlockIDPtr, std::string> result);
#if BUILDFLAG(ENABLE_ORCHARD)
  void OnGetTreeState(
      ParamsBundle params,
      base::expected<zcash::mojom::TreeStatePtr, std::string> result);
  void OnSignOrchardPartComplete(
      ParamsBundle params,
      std::unique_ptr<OrchardBundleManager> orchard_bundle_manager);
#endif  // BUILDFLAG(ENABLE_ORCHARD)

  raw_ptr<ZCashWalletService> zcash_wallet_service_;  // Owns `this`.
  base::WeakPtrFactory<ZCashTransactionCompleteManager> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TRANSACTION_COMPLETE_MANAGER_H_
