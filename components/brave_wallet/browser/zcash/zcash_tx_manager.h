/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/scoped_observation.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_block_tracker.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"

namespace brave_wallet {

class AccountResolverDelegate;
class TxService;
class KeyringService;
class ZCashWalletService;
class ZCashTxStateManager;

class ZCashTxManager : public TxManager, public ZCashBlockTracker::Observer {
 public:
  ZCashTxManager(TxService& tx_service,
                 ZCashWalletService& bitcoin_wallet_service,
                 KeyringService& keyring_service,
                 TxStorageDelegate& delegate,
                 AccountResolverDelegate& account_resolver_delegate);
  ~ZCashTxManager() override;
  ZCashTxManager(const ZCashTxManager&) = delete;
  ZCashTxManager& operator=(const ZCashTxManager&) = delete;

 private:
  friend class BraveWalletP3AUnitTest;

  ZCashTxStateManager& GetZCashTxStateManager();
  ZCashBlockTracker& GetZCashBlockTracker();

  // ZCashBlockTracker::Observer
  void OnLatestHeightUpdated(const std::string& chain_id,
                             uint32_t latest_height) override;

  // TxManager
  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataUnionPtr tx_data_union,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                AddUnapprovedTransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;

  void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) override;
  void RetryTransaction(const std::string& tx_meta_id,
                        RetryTransactionCallback callback) override;
  mojom::CoinType GetCoinType() const override;
  void UpdatePendingTransactions(
      const std::optional<std::string>& chain_id) override;

  void ContinueAddUnapprovedTransaction(
      const std::string& chain_id,
      const mojom::AccountIdPtr& from,
      const std::optional<url::Origin>& origin,
      AddUnapprovedTransactionCallback callback,
      base::expected<ZCashTransaction, std::string> zcash_transaction);

  void ContinueApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback callback,
                                  std::string tx_cid,
                                  ZCashTransaction transaction,
                                  std::string error);

  void OnGetTransactionStatus(const std::string& tx_meta_id,
                              base::expected<bool, std::string> confirm_status);

  raw_ref<ZCashWalletService> zcash_wallet_service_;
  base::ScopedObservation<ZCashBlockTracker, ZCashBlockTracker::Observer>
      block_tracker_observation_{this};
  base::WeakPtrFactory<ZCashTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_MANAGER_H_
