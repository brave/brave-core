/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"

namespace brave_wallet {

EthTxController::EthTxController(
    EthJsonRpcController* rpc_controller,
    KeyringController* keyring_controller,
    std::unique_ptr<EthTxStateManager> tx_state_manager,
    std::unique_ptr<EthNonceTracker> nonce_tracker,
    std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
    PrefService* prefs)
    : rpc_controller_(rpc_controller),
      keyring_controller_(keyring_controller),
      tx_state_manager_(std::move(tx_state_manager)),
      nonce_tracker_(std::move(nonce_tracker)),
      pending_tx_tracker_(std::move(pending_tx_tracker)),
      weak_factory_(this) {
  DCHECK(rpc_controller_);
  DCHECK(keyring_controller_);
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

void EthTxController::AddUnapprovedTransaction(
    mojom::TxDataPtr tx_data,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  auto tx = EthTransaction::FromTxData(tx_data);
  if (!tx) {
    std::move(callback).Run(false, "");
    return;
  }
  EthTxStateManager::TxMeta meta(std::make_unique<EthTransaction>(*tx));
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.from = EthAddress::FromHex(from);
  meta.created_time = base::Time::Now();
  meta.status = mojom::TransactionStatus::Unapproved;
  tx_state_manager_->AddOrUpdateTx(meta);

  for (const auto& observer : observers_)
    observer->OnNewUnapprovedTx(
        EthTxStateManager::TxMetaToTransactionInfo(meta));

  std::move(callback).Run(true, meta.id);
}

void EthTxController::AddUnapproved1559Transaction(
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    AddUnapproved1559TransactionCallback callback) {
  auto tx = Eip1559Transaction::FromTxData(tx_data);
  if (!tx) {
    std::move(callback).Run(false, "");
    return;
  }
  EthTxStateManager::TxMeta meta(std::make_unique<Eip1559Transaction>(*tx));
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.from = EthAddress::FromHex(from);
  meta.created_time = base::Time::Now();
  meta.status = mojom::TransactionStatus::Unapproved;
  tx_state_manager_->AddOrUpdateTx(meta);

  for (const auto& observer : observers_)
    observer->OnNewUnapprovedTx(
        EthTxStateManager::TxMetaToTransactionInfo(meta));

  std::move(callback).Run(true, meta.id);
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

  uint256_t chain_id = 0;
  if (!HexValueToUint256(rpc_controller_->GetChainId(), &chain_id)) {
    LOG(ERROR) << "Could not convert chain ID";
    std::move(callback).Run(false);
    return;
  }

  if (!meta->last_gas_price) {
    nonce_tracker_->GetNextNonce(
        meta->from,
        base::BindOnce(&EthTxController::OnGetNextNonce,
                       weak_factory_.GetWeakPtr(), std::move(meta), chain_id));
  } else {
    uint256_t nonce = meta->tx->nonce();
    OnGetNextNonce(std::move(meta), chain_id, true, nonce);
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
  meta->status = mojom::TransactionStatus::Rejected;
  tx_state_manager_->AddOrUpdateTx(*meta);
  NotifyTransactionStatusChanged(meta.get());
  std::move(callback).Run(true);
}

void EthTxController::OnGetNextNonce(
    std::unique_ptr<EthTxStateManager::TxMeta> meta,
    uint256_t chain_id,
    bool success,
    uint256_t nonce) {
  if (!success) {
    meta->status = mojom::TransactionStatus::Error;
    tx_state_manager_->AddOrUpdateTx(*meta);
    NotifyTransactionStatusChanged(meta.get());
    LOG(ERROR) << "GetNextNonce failed";
    return;
  }
  meta->tx->set_nonce(nonce);
  DCHECK(!keyring_controller_->IsLocked());
  keyring_controller_->SignTransactionByDefaultKeyring(
      meta->from.ToChecksumAddress(), meta->tx.get(), chain_id);
  meta->status = mojom::TransactionStatus::Approved;
  tx_state_manager_->AddOrUpdateTx(*meta);
  NotifyTransactionStatusChanged(meta.get());
  if (!meta->tx->IsSigned()) {
    LOG(ERROR) << "Transaction must be signed first";
    return;
  }
  PublishTransaction(meta->id, meta->tx->GetSignedTransaction());
}

void EthTxController::PublishTransaction(
    const std::string& tx_meta_id,
    const std::string& signed_transaction) {
  rpc_controller_->SendRawTransaction(
      signed_transaction,
      base::BindOnce(&EthTxController::OnPublishTransaction,
                     weak_factory_.GetWeakPtr(), tx_meta_id));
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
    meta->status = mojom::TransactionStatus::Submitted;
    meta->submitted_time = base::Time::Now();
    meta->tx_hash = tx_hash;
  } else {
    meta->status = mojom::TransactionStatus::Error;
  }

  tx_state_manager_->AddOrUpdateTx(*meta);
  NotifyTransactionStatusChanged(meta.get());

  if (status) {
    pending_tx_tracker_->UpdatePendingTransactions();
  }
}

void EthTxController::MakeERC20TransferData(
    const std::string& to_address,
    const std::string& amount,
    MakeERC20TransferDataCallback callback) {
  uint256_t amount_uint = 0;
  if (!HexValueToUint256(amount, &amount_uint)) {
    LOG(ERROR) << "Could not convert amount";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::string data;
  if (!erc20::Transfer(to_address, amount_uint, &data)) {
    LOG(ERROR) << "Could not make transfer data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  if (!base::StartsWith(data, "0x")) {
    LOG(ERROR) << "Invalid format returned from Transfer";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::vector<uint8_t> data_decoded;
  if (!base::HexStringToBytes(data.data() + 2, &data_decoded)) {
    LOG(ERROR) << "Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxController::MakeERC20ApproveData(
    const std::string& spender_address,
    const std::string& amount,
    MakeERC20ApproveDataCallback callback) {
  uint256_t amount_uint = 0;
  if (!HexValueToUint256(amount, &amount_uint)) {
    LOG(ERROR) << "Could not convert amount";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::string data;
  if (!erc20::Approve(spender_address, amount_uint, &data)) {
    LOG(ERROR) << "Could not make transfer data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  if (!base::StartsWith(data, "0x")) {
    LOG(ERROR) << "Invalid format returned from Transfer";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::vector<uint8_t> data_decoded;
  if (!base::HexStringToBytes(data.data() + 2, &data_decoded)) {
    LOG(ERROR) << "Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxController::AddObserver(
    ::mojo::PendingRemote<mojom::EthTxControllerObserver> observer) {
  observers_.Add(std::move(observer));
}

void EthTxController::NotifyTransactionStatusChanged(
    EthTxStateManager::TxMeta* meta) {
  for (const auto& observer : observers_)
    observer->OnTransactionStatusChanged(
        EthTxStateManager::TxMetaToTransactionInfo(*meta));
}

void EthTxController::GetAllTransactionInfo(
    const std::string& from,
    GetAllTransactionInfoCallback callback) {
  auto from_address = EthAddress::FromHex(from);
  if (from_address.IsEmpty()) {
    std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
    return;
  }
  std::vector<std::unique_ptr<EthTxStateManager::TxMeta>> metas =
      tx_state_manager_->GetTransactionsByStatus(absl::nullopt, from_address);

  // Convert vector of TxMeta to vector of TransactionInfo
  std::vector<mojom::TransactionInfoPtr> tis(metas.size());
  std::transform(metas.begin(), metas.end(), tis.begin(),
                 [](const std::unique_ptr<EthTxStateManager::TxMeta>& m)
                     -> mojom::TransactionInfoPtr {
                   return EthTxStateManager::TxMetaToTransactionInfo(*m);
                 });
  std::move(callback).Run(std::move(tis));
}

}  // namespace brave_wallet
