/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"

namespace brave_wallet {

EthTxController::EthTxController(
    mojo::PendingRemote<mojom::EthJsonRpcController>
        eth_json_rpc_controller_pending,
    mojo::PendingRemote<mojom::KeyringController> keyring_controller_pending,
    std::unique_ptr<EthTxStateManager> tx_state_manager,
    std::unique_ptr<EthNonceTracker> nonce_tracker,
    std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
    PrefService* prefs)
    : tx_state_manager_(std::move(tx_state_manager)),
      nonce_tracker_(std::move(nonce_tracker)),
      pending_tx_tracker_(std::move(pending_tx_tracker)),
      weak_factory_(this) {
  eth_json_rpc_controller_.Bind(std::move(eth_json_rpc_controller_pending));
  DCHECK(eth_json_rpc_controller_);
  eth_json_rpc_controller_.set_disconnect_handler(base::BindOnce(
      &EthTxController::OnConnectionError, weak_factory_.GetWeakPtr()));

  keyring_controller_.Bind(std::move(keyring_controller_pending));
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(base::BindOnce(
      &EthTxController::OnConnectionError, weak_factory_.GetWeakPtr()));
}

EthTxController::~EthTxController() = default;

mojo::PendingRemote<mojom::EthTxController> EthTxController::MakeRemote() {
  mojo::PendingRemote<mojom::EthTxController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void EthTxController::Bind(
    mojo::PendingReceiver<mojom::EthTxController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void EthTxController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void EthTxController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void EthTxController::AddUnapprovedTransaction(
    std::unique_ptr<EthTransaction> tx,
    const EthAddress& from) {
  EthTxStateManager::TxMeta meta(std::move(tx));
  meta.id = EthTxStateManager::GenerateMetaID();
  // TODO(darkdh): estimate gas price and limit
  meta.tx->set_gas_price("0x14");
  meta.tx->set_gas_limit("0x5208");
  meta.from = from;
  meta.created_time = base::Time::Now();
  meta.status = EthTxStateManager::TransactionStatus::UNAPPROVED;
  tx_state_manager_->AddOrUpdateTx(meta);

  for (Observer& observer : observers_)
    observer.OnNewUnapprovedTx(meta);
}

void EthTxController::ApproveTransaction(const std::string& tx_meta_id,
                                         ApproveTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(false);
    return;
  }
  if (!meta->last_gas_price) {
    nonce_tracker_->GetNextNonce(
        meta->from,
        base::BindOnce(&EthTxController::OnGetNextNonce,
                       weak_factory_.GetWeakPtr(), std::move(meta)));
  } else {
    OnGetNextNonce(std::move(meta), true, meta->tx->nonce());
  }

  std::move(callback).Run(true);
}

void EthTxController::RejectTransaction(const std::string& tx_meta_id,
                                        RejectTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(false);
    return;
  }
  meta->status = EthTxStateManager::TransactionStatus::REJECTED;
  tx_state_manager_->AddOrUpdateTx(*meta);
  std::move(callback).Run(true);
}

void EthTxController::OnGetNextNonce(
    std::unique_ptr<EthTxStateManager::TxMeta> meta,
    bool success,
    const std::string& nonce) {
  if (!success) {
    // TODO(darkdh): Notify observers
    LOG(ERROR) << "GetNextNonce failed";
    return;
  }

  meta->tx->set_nonce(nonce);

  if (!keyring_controller_) {
    LOG(ERROR)
        << "Keyring controller is not set so can't update tx state manager";
    return;
  }
  keyring_controller_->IsLocked(
      base::BindOnce([](bool locked) { DCHECK(!locked); }));
  keyring_controller_->SignTransactionByDefaultKeyring(
      meta->from.ToChecksumAddress(), meta->tx->MakeRemote(),
      base::BindOnce(
          [](EthTxStateManager::TxMeta* meta,
             EthTxStateManager* tx_state_manager, EthTxController* controller,
             bool success) {
            if (!success)
              return;
            meta->status = EthTxStateManager::TransactionStatus::APPROVED;
            tx_state_manager->AddOrUpdateTx(*meta);
            controller->PublishTransaction(meta->tx.get(), meta->id);
          },
          meta.get(), tx_state_manager_.get(), this));
}

void EthTxController::PublishTransaction(EthTransaction* tx,
                                         const std::string& tx_meta_id) {
  CHECK(tx);
  if (!tx->IsSigned()) {
    LOG(ERROR) << "Transaction must be signed first";
    return;
  }

  if (!eth_json_rpc_controller_) {
    LOG(ERROR)
        << "eth json rpc controller not set, cannot send raw transaction";
    return;
  }

  tx->GetSignedTransaction(base::BindOnce(
      [](base::WeakPtr<brave_wallet::EthTxController> weak_ptr,
         const std::string& tx_meta_id,
         mojo::Remote<mojom::EthJsonRpcController>* eth_json_rpc_controller,
         bool status, const std::string& signed_transaction) {
        if (!eth_json_rpc_controller) {
          return;
        }

        if (!status) {
          LOG(ERROR) << "Could not publish transaction because transaction is "
                        "not signed";
          return;
        }

        (*eth_json_rpc_controller)
            ->SendRawTransaction(
                signed_transaction,
                base::BindOnce(&EthTxController::OnPublishTransaction, weak_ptr,
                               tx_meta_id));
      },
      weak_factory_.GetWeakPtr(), tx_meta_id, &eth_json_rpc_controller_));
}

void EthTxController::OnPublishTransaction(std::string tx_meta_id,
                                           bool status,
                                           const std::string& tx_hash) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    return;
  }
  if (status) {
    meta->status = EthTxStateManager::TransactionStatus::SUBMITTED;
    meta->submitted_time = base::Time::Now();
    meta->tx_hash = tx_hash;
    tx_state_manager_->AddOrUpdateTx(*meta);
    pending_tx_tracker_->UpdatePendingTransactions();
  }
}

void EthTxController::OnConnectionError() {
  eth_json_rpc_controller_.reset();
  keyring_controller_.reset();
}

}  // namespace brave_wallet
