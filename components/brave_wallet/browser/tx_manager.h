/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace brave_wallet {

class BlockTracker;
class KeyringService;
class TxService;

class TxManager : public TxStateManager::Observer,
                  public KeyringServiceObserverBase {
 public:
  TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
            std::unique_ptr<BlockTracker> block_tracker,
            TxService* tx_service,
            KeyringService* keyring_service,
            PrefService* prefs);
  ~TxManager() override;

  using AddUnapprovedTransactionCallback =
      mojom::TxService::AddUnapprovedTransactionCallback;
  using ApproveTransactionCallback =
      mojom::TxService::ApproveTransactionCallback;
  using RejectTransactionCallback = mojom::TxService::RejectTransactionCallback;
  using GetTransactionInfoCallback =
      mojom::TxService::GetTransactionInfoCallback;
  using SpeedupOrCancelTransactionCallback =
      mojom::TxService::SpeedupOrCancelTransactionCallback;
  using RetryTransactionCallback = mojom::TxService::RetryTransactionCallback;
  using GetTransactionMessageToSignCallback =
      mojom::TxService::GetTransactionMessageToSignCallback;

  virtual void AddUnapprovedTransaction(
      const std::string& chain_id,
      mojom::TxDataUnionPtr tx_data_union,
      const mojom::AccountIdPtr& from,
      const absl::optional<url::Origin>& origin,
      AddUnapprovedTransactionCallback) = 0;
  virtual void ApproveTransaction(const std::string& chain_id,
                                  const std::string& tx_meta_id,
                                  ApproveTransactionCallback) = 0;
  virtual void RejectTransaction(const std::string& chain_id,
                                 const std::string& tx_meta_id,
                                 RejectTransactionCallback);
  virtual void GetTransactionInfo(const std::string& chain_id,
                                  const std::string& tx_meta_id,
                                  GetTransactionInfoCallback);
  std::vector<mojom::TransactionInfoPtr> GetAllTransactionInfo(
      const absl::optional<std::string>& chain_id,
      const absl::optional<mojom::AccountIdPtr>& from);

  virtual void SpeedupOrCancelTransaction(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) = 0;
  virtual void RetryTransaction(const std::string& chain_id,
                                const std::string& tx_meta_id,
                                RetryTransactionCallback callback) = 0;

  virtual void GetTransactionMessageToSign(
      const std::string& chain_id,
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) = 0;

  virtual void Reset();

 protected:
  void CheckIfBlockTrackerShouldRun(
      const std::set<std::string>& new_pending_chain_ids);
  virtual void UpdatePendingTransactions(
      const absl::optional<std::string>& chain_id) = 0;

  std::unique_ptr<TxStateManager> tx_state_manager_;
  std::unique_ptr<BlockTracker> block_tracker_;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  std::set<std::string> pending_chain_ids_;

 private:
  virtual mojom::CoinType GetCoinType() const = 0;

  // TxStateManager::Observer
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override;

  // mojom::KeyringServiceObserverBase:
  void WalletReset() override;
  void Locked() override;
  void Unlocked() override;

  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
