/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_

#include <memory>
#include <string>

#include "base/observer_list.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_wallet {

class EthJsonRpcController;
class KeyringController;

class EthTxController : public KeyedService, public mojom::EthTxController {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnNewUnapprovedTx(const EthTxStateManager::TxMeta&) = 0;

   protected:
    ~Observer() override = default;
  };
  explicit EthTxController(
      mojo::PendingRemote<mojom::EthJsonRpcController>
          eth_json_rpc_controller_pending,
      mojo::PendingRemote<mojom::KeyringController> keyring_controller_pending,
      std::unique_ptr<EthTxStateManager> tx_state_manager,
      std::unique_ptr<EthNonceTracker> nonce_tracker,
      std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
      PrefService* prefs);
  ~EthTxController() override;
  EthTxController(const EthTxController&) = delete;
  EthTxController operator=(const EthTxController&) = delete;

  mojo::PendingRemote<mojom::EthTxController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthTxController> receiver);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void AddUnapprovedTransaction(std::unique_ptr<EthTransaction> tx,
                                const EthAddress& from);
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(const std::string& tx_meta_id,
                         RejectTransactionCallback) override;

 private:
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxStateManager::TxMeta> meta,
                      bool success,
                      const std::string& nonce);
  void PublishTransaction(EthTransaction* tx, const std::string& tx_meta_id);
  void OnPublishTransaction(std::string tx_meta_id,
                            bool status,
                            const std::string& tx_hash);

  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;

  mojo::Remote<mojom::EthJsonRpcController> eth_json_rpc_controller_;
  mojo::Remote<mojom::KeyringController> keyring_controller_;

  base::ObserverList<Observer> observers_;
  mojo::ReceiverSet<mojom::EthTxController> receivers_;

  base::WeakPtrFactory<EthTxController> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
