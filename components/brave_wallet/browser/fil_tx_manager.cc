/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include <memory>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"

namespace brave_wallet {

FilTxManager::FilTxManager(TxService* tx_service,
                           JsonRpcService* json_rpc_service,
                           KeyringService* keyring_service,
                           PrefService* prefs)
    : TxManager(std::make_unique<FilTxStateManager>(prefs, json_rpc_service),
                std::make_unique<FilBlockTracker>(json_rpc_service),
                tx_service,
                json_rpc_service,
                keyring_service,
                prefs),
      nonce_tracker_(std::make_unique<FilNonceTracker>(GetFilTxStateManager(),
                                                       json_rpc_service)) {}

FilTxManager::~FilTxManager() {}

void FilTxManager::AddUnapprovedTransaction(
    mojom::FilTxDataPtr tx_data,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  if (!FilAddress::IsValidAddress(from)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_FIL_SEND_TRANSACTION_FROM_INVALID));
    return;
  }
  if (!FilAddress::IsValidAddress(tx_data->to)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_FIL_SEND_TRANSACTION_TO));
    return;
  }
  if (!tx_data->to.empty() && !FilAddress::IsValidAddress(tx_data->to)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_TO_INVALID));

    return;
  }

  auto tx = FilTransaction::FromTxData(tx_data);
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }
  auto tx_ptr = std::make_unique<FilTransaction>(*tx);
  const std::string gas_fee_cap = tx->gas_fee_cap();
  const std::string gas_premium = tx->gas_premium();
  auto gas_limit = tx->gas_limit();
  const std::string cid = tx->cid();

  if (!gas_limit || gas_fee_cap.empty() || gas_premium.empty() || cid.empty()) {
    GetEstimatedGas(from, std::move(tx_ptr), std::move(callback));
  } else {
    ContinueAddUnapprovedTransaction(
        from, std::move(tx_ptr), std::move(callback), gas_premium, gas_fee_cap,
        gas_limit, cid, mojom::ProviderError::kSuccess, "");
  }
}

void FilTxManager::GetEstimatedGas(const std::string& from,
                                   std::unique_ptr<FilTransaction> tx,
                                   AddUnapprovedTransactionCallback callback) {
  const std::string gas_premium = tx->gas_premium();
  const std::string gas_fee_cap = tx->gas_fee_cap();
  auto gas_limit = tx->gas_limit();
  uint64_t nonce = tx->nonce().value();
  const std::string value = tx->value();
  const std::string max_fee = tx->max_fee();
  auto to = tx->to().ToChecksumAddress();
  json_rpc_service_->GetFilEstimateGas(
      from, to, gas_premium, gas_fee_cap, gas_limit, nonce, max_fee, value,
      base::BindOnce(&FilTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), from, std::move(tx),
                     std::move(callback)));
}

void FilTxManager::ContinueAddUnapprovedTransaction(
    const std::string& from,
    std::unique_ptr<FilTransaction> tx,
    AddUnapprovedTransactionCallback callback,
    const std::string& gas_premium,
    const std::string& gas_fee_cap,
    uint64_t gas_limit,
    const std::string& cid,
    mojom::ProviderError error,
    const std::string& error_message) {
  tx->set_gas_premium(gas_premium);
  tx->set_fee_cap(gas_fee_cap);
  tx->set_gas_limit(gas_limit);
  tx->set_cid(cid);
  FilTxMeta meta(std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_from(FilAddress::FromAddress(from).ToChecksumAddress());
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  GetFilTxStateManager()->AddOrUpdateTx(meta);
  std::move(callback).Run(true, meta.id(), "");
}

void FilTxManager::AddUnapprovedTransaction(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  AddUnapprovedTransaction(std::move(tx_data_union->get_fil_tx_data()), from,
                           std::move(callback));
}

void FilTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::RejectTransaction(const std::string& tx_meta_id,
                                     RejectTransactionCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::GetAllTransactionInfo(
    const std::string& from,
    GetAllTransactionInfoCallback callback) {
  auto from_address = FilAddress::FromAddress(from);
  if (from_address.IsEmpty()) {
    std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
    return;
  }
  TxManager::GetAllTransactionInfo(from_address.ToChecksumAddress(),
                                   std::move(callback));
}

void FilTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::RetryTransaction(const std::string& tx_meta_id,
                                    RetryTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::GetTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::Reset() {
  TxManager::Reset();
  // TODO(spylogsster): reset members as necessary.
}

void FilTxManager::UpdatePendingTransactions() {
  NOTIMPLEMENTED();
}

}  // namespace brave_wallet
