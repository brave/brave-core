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

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_wallet {

class EthJsonRpcController;
class KeyringController;

class EthTxController {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnNewUnapprovedTx(const EthTxStateManager::TxMeta&) = 0;

   protected:
    ~Observer() override = default;
  };
  EthTxController(EthJsonRpcController*, KeyringController*, PrefService*);
  ~EthTxController();
  EthTxController(const EthTxController&) = delete;
  EthTxController operator=(const EthTxController&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void AddUnapprovedTransaction(const EthTransaction& tx,
                                const EthAddress& from);
  bool ApproveTransaction(const std::string& tx_meta_id);
  void RejectTransaction(const std::string& tx_meta_id);

 private:
  void OnGetNextNonce(EthTxStateManager::TxMeta meta,
                      bool success,
                      uint256_t nonce);
  void PublishTransaction(const EthTransaction& tx,
                          const std::string& tx_meta_id);
  void OnPublishTransaction(std::string tx_meta_id,
                            bool status,
                            const std::string& tx_hash);

  EthJsonRpcController* rpc_controller_;
  KeyringController* keyring_controller_;

  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<EthTxController> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
