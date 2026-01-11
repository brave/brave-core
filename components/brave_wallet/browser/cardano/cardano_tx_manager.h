/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_MANAGER_H_

#include <memory>
#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_block_tracker.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/browser/tx_manager.h"

namespace brave_wallet {

class AccountResolverDelegate;
class TxService;
class KeyringService;
class CardanoWalletService;
class CardanoTxStateManager;

class CardanoTxManager : public TxManager,
                         public CardanoBlockTracker::Observer {
 public:
  using AddUnapprovedCardanoTransactionCallback =
      mojom::TxService::AddUnapprovedCardanoTransactionCallback;

  CardanoTxManager(TxService& tx_service,
                   CardanoWalletService& cardano_wallet_service,
                   KeyringService& keyring_service,
                   TxStorageDelegate& delegate,
                   AccountResolverDelegate& account_resolver_delegate);
  ~CardanoTxManager() override;
  CardanoTxManager(const CardanoTxManager&) = delete;
  CardanoTxManager& operator=(const CardanoTxManager&) = delete;
  std::unique_ptr<CardanoTxMeta> GetTxForTesting(const std::string& tx_meta_id);

  void AddUnapprovedCardanoTransaction(
      mojom::NewCardanoTransactionParamsPtr params,
      AddUnapprovedCardanoTransactionCallback callback);

 private:
  friend class CardanoTxManagerUnitTest;
  FRIEND_TEST_ALL_PREFIXES(CardanoTxManagerUnitTest, SubmitTransaction);
  FRIEND_TEST_ALL_PREFIXES(CardanoTxManagerUnitTest, SubmitTransactionError);
  FRIEND_TEST_ALL_PREFIXES(CardanoTxManagerUnitTest, SubmitTransactionError);

  CardanoTxStateManager& GetCardanoTxStateManager();
  CardanoBlockTracker& GetCardanoBlockTracker();

  // CardanoBlockTracker::Observer
  void OnLatestHeightUpdated(const std::string& chain_id,
                             uint32_t latest_height) override;

  // TxManager
  void AddUnapprovedTransaction(const std::string& chain_id,
                                mojom::TxDataUnionPtr tx_data_union,
                                const mojom::AccountIdPtr& from,
                                const std::optional<url::Origin>& origin,
                                mojom::SwapInfoPtr swap_info,
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
      mojom::SwapInfoPtr swap_info,
      AddUnapprovedTransactionCallback callback,
      base::expected<CardanoTransaction, std::string> cardano_transaction);

  void ContinueApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback callback,
                                  std::string tx_cid,
                                  CardanoTransaction transaction,
                                  std::string error);

  void OnGetTransactionStatus(const std::string& tx_meta_id,
                              base::expected<bool, std::string> confirm_status);

  const raw_ref<CardanoWalletService> cardano_wallet_service_;
  base::ScopedObservation<CardanoBlockTracker, CardanoBlockTracker::Observer>
      block_tracker_observation_{this};
  base::WeakPtrFactory<CardanoTxManager> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_MANAGER_H_
