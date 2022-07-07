/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

class PrefService;

namespace brave_wallet {

class BlockTracker;
class JsonRpcService;
class KeyringService;
class TxService;

class TxManager : public TxStateManager::Observer,
                  public mojom::KeyringServiceObserver {
 public:
  TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
            std::unique_ptr<BlockTracker> block_tracker,
            TxService* tx_service,
            JsonRpcService* json_rpc_service,
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
  using GetAllTransactionInfoCallback =
      mojom::TxService::GetAllTransactionInfoCallback;
  using SpeedupOrCancelTransactionCallback =
      mojom::TxService::SpeedupOrCancelTransactionCallback;
  using RetryTransactionCallback = mojom::TxService::RetryTransactionCallback;
  using GetTransactionMessageToSignCallback =
      mojom::TxService::GetTransactionMessageToSignCallback;

  virtual void AddUnapprovedTransaction(
      mojom::TxDataUnionPtr tx_data_union,
      const std::string& from,
      const absl::optional<url::Origin>& origin,
      const absl::optional<std::string>& group_id,
      AddUnapprovedTransactionCallback) = 0;
  virtual void ApproveTransaction(const std::string& tx_meta_id,
                                  ApproveTransactionCallback) = 0;
  virtual void RejectTransaction(const std::string& tx_meta_id,
                                 RejectTransactionCallback);
  virtual void GetTransactionInfo(const std::string& tx_meta_id,
                                  GetTransactionInfoCallback);
  virtual void GetAllTransactionInfo(const std::string& from,
                                     GetAllTransactionInfoCallback);

  virtual void SpeedupOrCancelTransaction(
      const std::string& tx_meta_id,
      bool cancel,
      SpeedupOrCancelTransactionCallback callback) = 0;
  virtual void RetryTransaction(const std::string& tx_meta_id,
                                RetryTransactionCallback callback) = 0;

  virtual void GetTransactionMessageToSign(
      const std::string& tx_meta_id,
      GetTransactionMessageToSignCallback callback) = 0;

  virtual void Reset();

 protected:
  void CheckIfBlockTrackerShouldRun();
  virtual void UpdatePendingTransactions() = 0;

  std::unique_ptr<TxStateManager> tx_state_manager_;
  std::unique_ptr<BlockTracker> block_tracker_;
  raw_ptr<TxService> tx_service_ = nullptr;             // NOT OWNED
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;  // NOT OWNED
  raw_ptr<KeyringService> keyring_service_ = nullptr;   // NOT OWNED
  raw_ptr<PrefService> prefs_ = nullptr;                // NOT OWNED
  bool known_no_pending_tx_ = false;

 private:
  // TxStateManager::Observer
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override;

  // mojom::KeyringServiceObserver
  void KeyringCreated(const std::string& keyring_id) override;
  void KeyringRestored(const std::string& keyring_id) override;
  void KeyringReset() override;
  void Locked() override;
  void Unlocked() override;
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged(mojom::CoinType coin) override {}

  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_MANAGER_H_
