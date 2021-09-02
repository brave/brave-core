/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace brave_wallet {

class EthJsonRpcController;
class KeyringController;

class EthTxController : public KeyedService, public mojom::EthTxController {
 public:
  explicit EthTxController(
      EthJsonRpcController* eth_json_rpc_controller,
      KeyringController* keyring_controller,
      std::unique_ptr<EthTxStateManager> tx_state_manager,
      std::unique_ptr<EthNonceTracker> nonce_tracker,
      std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
      PrefService* prefs);
  ~EthTxController() override;
  EthTxController(const EthTxController&) = delete;
  EthTxController operator=(const EthTxController&) = delete;

  mojo::PendingRemote<mojom::EthTxController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthTxController> receiver);

  void AddUnapprovedTransaction(mojom::TxDataPtr tx_data,
                                const std::string& from,
                                AddUnapprovedTransactionCallback) override;
  void AddUnapproved1559Transaction(
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      AddUnapproved1559TransactionCallback) override;
  void ApproveTransaction(const std::string& tx_meta_id,
                          ApproveTransactionCallback) override;
  void RejectTransaction(const std::string& tx_meta_id,
                         RejectTransactionCallback) override;
  void MakeERC20TransferData(const std::string& to_address,
                             const std::string& amount,
                             MakeERC20TransferDataCallback) override;
  void MakeERC20ApproveData(const std::string& to_address,
                            const std::string& amount,
                            MakeERC20ApproveDataCallback) override;
  void GetAllTransactionInfo(const std::string& from,
                             GetAllTransactionInfoCallback) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::EthTxControllerObserver> observer) override;

 private:
  void NotifyTransactionStatusChanged(EthTxStateManager::TxMeta* meta);
  void OnConnectionError();
  void OnGetNextNonce(std::unique_ptr<EthTxStateManager::TxMeta> meta,
                      uint256_t chain_id,
                      bool success,
                      uint256_t nonce);
  void PublishTransaction(const std::string& tx_meta_id,
                          const std::string& signed_transaction);
  void OnPublishTransaction(std::string tx_meta_id,
                            bool status,
                            const std::string& tx_hash);

  EthJsonRpcController* rpc_controller_;   // NOT OWNED
  KeyringController* keyring_controller_;  // NOT OWNED
  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  std::unique_ptr<EthNonceTracker> nonce_tracker_;
  std::unique_ptr<EthPendingTxTracker> pending_tx_tracker_;

  mojo::RemoteSet<mojom::EthTxControllerObserver> observers_;
  mojo::ReceiverSet<mojom::EthTxController> receivers_;

  base::WeakPtrFactory<EthTxController> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_CONTROLLER_H_
