/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_controller.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

// static
bool EthTxController::ValidateTxData(const mojom::TxDataPtr& tx_data,
                                     std::string* error) {
  CHECK(error);
  // To cannot be empty if data is not specified
  if (tx_data->data.size() == 0 && tx_data->to.empty()) {
    *error =
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_TO_OR_DATA);
    return false;
  }

  // If the following fields are specified, they must be valid hex strings
  if (!tx_data->nonce.empty() && !IsValidHexString(tx_data->nonce)) {
    *error =
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NONCE_INVALID);
    return false;
  }
  if (!tx_data->gas_price.empty() && !IsValidHexString(tx_data->gas_price)) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_GAS_PRICE_INVALID);
    return false;
  }
  if (!tx_data->gas_limit.empty() && !IsValidHexString(tx_data->gas_limit)) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_GAS_LIMIT_INVALID);
    return false;
  }
  if (!tx_data->value.empty() && !IsValidHexString(tx_data->value)) {
    *error =
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_VALUE_INVALID);
    return false;
  }
  // to must be a valid address if specified
  if (!tx_data->to.empty() && EthAddress::FromHex(tx_data->to).IsEmpty()) {
    *error =
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_TO_INVALID);
    return false;
  }
  return true;
}

// static
bool EthTxController::ValidateTxData1559(const mojom::TxData1559Ptr& tx_data,
                                         std::string* error) {
  if (!ValidateTxData(tx_data->base_data, error))
    return false;
  // Not allowed to have empty gas price and fee per gas
  if (!tx_data->base_data->gas_price.empty() &&
      !tx_data->max_fee_per_gas.empty()) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_GAS_PRICING_EXISTS);
    return false;
  }
  // If the following fields are specified, they must be valid hex strings
  if (!tx_data->chain_id.empty() && !IsValidHexString(tx_data->chain_id)) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_CHAIN_ID_INVALID);
    return false;
  }
  if (!tx_data->max_priority_fee_per_gas.empty() &&
      !IsValidHexString(tx_data->max_priority_fee_per_gas)) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_MAX_PRIORITY_FEE_PER_GAS_INVALID);
    return false;
  }
  if (!tx_data->max_fee_per_gas.empty() &&
      !IsValidHexString(tx_data->max_fee_per_gas)) {
    *error = l10n_util::GetStringUTF8(
        IDS_WALLET_ETH_SEND_TRANSACTION_MAX_FEE_PER_GAS_INVALID);
    return false;
  }

  return true;
}

EthTxController::EthTxController(
    EthJsonRpcController* rpc_controller,
    KeyringController* keyring_controller,
    AssetRatioController* asset_ratio_controller,
    std::unique_ptr<EthTxStateManager> tx_state_manager,
    std::unique_ptr<EthNonceTracker> nonce_tracker,
    std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
    PrefService* prefs)
    : rpc_controller_(rpc_controller),
      keyring_controller_(keyring_controller),
      asset_ratio_controller_(asset_ratio_controller),
      tx_state_manager_(std::move(tx_state_manager)),
      nonce_tracker_(std::move(nonce_tracker)),
      pending_tx_tracker_(std::move(pending_tx_tracker)),
      eth_block_tracker_(std::make_unique<EthBlockTracker>(rpc_controller)),
      weak_factory_(this) {
  DCHECK(rpc_controller_);
  DCHECK(keyring_controller_);
  CheckIfBlockTrackerShouldRun();
  eth_block_tracker_->AddObserver(this);
  tx_state_manager_->AddObserver(this);
  keyring_controller_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

EthTxController::~EthTxController() {
  eth_block_tracker_->RemoveObserver(this);
  tx_state_manager_->RemoveObserver(this);
}

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
  if (from.empty()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_FROM_EMPTY));
    return;
  }
  std::string error;
  if (!EthTxController::ValidateTxData(tx_data, &error)) {
    std::move(callback).Run(false, "", error);
    return;
  }
  auto tx = EthTransaction::FromTxData(tx_data, false);
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }

  auto tx_ptr = std::make_unique<EthTransaction>(*tx);

  // Use default gas price and limit for ETHSend.
  mojom::TransactionType tx_type;
  if (!GetTransactionInfoFromData(ToHex(tx_ptr->data()), &tx_type, nullptr,
                                  nullptr)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_TX_TYPE_FAILED));
    return;
  }

  if (tx_type == mojom::TransactionType::ETHSend) {
    if (!tx_ptr->gas_limit())
      tx_ptr->set_gas_limit(kDefaultSendEthGasLimit);
    if (!tx_ptr->gas_price())
      tx_ptr->set_gas_price(kDefaultSendEthGasPrice);

    const std::string new_gas_limit = Uint256ValueToHex(tx_ptr->gas_limit());
    ContinueAddUnapprovedTransaction(from, std::move(tx_ptr),
                                     std::move(callback), true, new_gas_limit);
    return;
  }

  if (!tx_ptr->gas_price()) {
    rpc_controller_->GetGasPrice(base::BindOnce(
        &EthTxController::OnGetGasPrice, weak_factory_.GetWeakPtr(), from,
        tx_data->to, tx_data->value, ToHex(tx_data->data), tx_data->gas_limit,
        std::move(tx_ptr), std::move(callback)));
  } else if (!tx_ptr->gas_limit()) {
    rpc_controller_->GetEstimateGas(
        from, tx_data->to, "" /* gas */, "" /* gas_price */, tx_data->value,
        ToHex(tx_data->data),
        base::BindOnce(&EthTxController::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), from, std::move(tx_ptr),
                       std::move(callback)));
  } else {
    ContinueAddUnapprovedTransaction(
        from, std::move(tx_ptr), std::move(callback), true, tx_data->gas_limit);
  }
}

void EthTxController::OnGetGasPrice(const std::string& from,
                                    const std::string& to,
                                    const std::string& value,
                                    const std::string& data,
                                    const std::string& gas_limit,
                                    std::unique_ptr<EthTransaction> tx,
                                    AddUnapprovedTransactionCallback callback,
                                    bool success,
                                    const std::string& result) {
  uint256_t gas_price;
  if (!success || !HexValueToUint256(result, &gas_price)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_PRICE_FAILED));
    return;
  }
  tx->set_gas_price(gas_price);

  if (!tx->gas_limit()) {
    rpc_controller_->GetEstimateGas(
        from, to, "" /* gas */, "" /* gas_price */, value, data,
        base::BindOnce(&EthTxController::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), from, std::move(tx),
                       std::move(callback)));
  } else {
    ContinueAddUnapprovedTransaction(from, std::move(tx), std::move(callback),
                                     true, gas_limit);
  }
}

void EthTxController::ContinueAddUnapprovedTransaction(
    const std::string& from,
    std::unique_ptr<EthTransaction> tx,
    AddUnapprovedTransactionCallback callback,
    bool success,
    const std::string& result) {
  uint256_t gas_limit;
  if (!success || !HexValueToUint256(result, &gas_limit)) {
    gas_limit = 0;
  }
  tx->set_gas_limit(gas_limit);

  EthTxStateManager::TxMeta meta(std::move(tx));
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.from = EthAddress::FromHex(from);
  meta.created_time = base::Time::Now();
  meta.status = mojom::TransactionStatus::Unapproved;
  tx_state_manager_->AddOrUpdateTx(meta);
  std::move(callback).Run(true, meta.id, "");
}

void EthTxController::AddUnapproved1559Transaction(
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    AddUnapproved1559TransactionCallback callback) {
  if (from.empty()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_FROM_EMPTY));
    return;
  }
  std::string error;
  if (!EthTxController::ValidateTxData1559(tx_data, &error)) {
    std::move(callback).Run(false, "", error);
    return;
  }
  auto tx = Eip1559Transaction::FromTxData(tx_data, false);
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }

  auto tx_ptr = std::make_unique<Eip1559Transaction>(*tx);

  mojom::TransactionType tx_type;
  if (!GetTransactionInfoFromData(ToHex(tx_ptr->data()), &tx_type, nullptr,
                                  nullptr)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_TX_TYPE_FAILED));
    return;
  }

  // Use default gas limit for ETHSend if it is empty.
  std::string gas_limit = tx_data->base_data->gas_limit;
  if (gas_limit.empty() && tx_type == mojom::TransactionType::ETHSend)
    gas_limit = Uint256ValueToHex(kDefaultSendEthGasLimit);

  if (!tx_ptr->max_priority_fee_per_gas() || !tx_ptr->max_fee_per_gas()) {
    asset_ratio_controller_->GetGasOracle(base::BindOnce(
        &EthTxController::OnGetGasOracle, weak_factory_.GetWeakPtr(), from,
        tx_data->base_data->to, tx_data->base_data->value,
        ToHex(tx_data->base_data->data), gas_limit, std::move(tx_ptr),
        std::move(callback)));
  } else if (gas_limit.empty()) {
    rpc_controller_->GetEstimateGas(
        from, tx_data->base_data->to, "" /* gas */, "" /* gas_price */,
        tx_data->base_data->value, ToHex(tx_data->base_data->data),
        base::BindOnce(&EthTxController::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), from, std::move(tx_ptr),
                       std::move(callback)));
  } else {
    ContinueAddUnapprovedTransaction(from, std::move(tx_ptr),
                                     std::move(callback), true, gas_limit);
  }
}

void EthTxController::OnGetGasOracle(
    const std::string& from,
    const std::string& to,
    const std::string& value,
    const std::string& data,
    const std::string& gas_limit,
    std::unique_ptr<Eip1559Transaction> tx,
    AddUnapprovedTransactionCallback callback,
    mojom::GasEstimation1559Ptr gas_estimation) {
  auto estimation =
      Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
          std::move(gas_estimation));
  if (!estimation) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_FEES_FAILED));
    return;
  }
  tx->set_gas_estimation(estimation.value());
  tx->set_max_fee_per_gas(estimation->avg_max_fee_per_gas);
  tx->set_max_priority_fee_per_gas(estimation->avg_max_priority_fee_per_gas);

  if (gas_limit.empty()) {
    rpc_controller_->GetEstimateGas(
        from, to, "" /* gas */, "" /* gas_price */, value, data,
        base::BindOnce(&EthTxController::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), from, std::move(tx),
                       std::move(callback)));
  } else {
    ContinueAddUnapprovedTransaction(from, std::move(tx), std::move(callback),
                                     true, gas_limit);
  }
}

void EthTxController::ApproveHardwareTransaction(
    const std::string& tx_meta_id,
    ApproveHardwareTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(false);
    return;
  }
  if (!meta->tx->nonce()) {
    auto from = EthAddress(meta->from);
    nonce_tracker_->GetNextNonce(
        from, base::BindOnce(&EthTxController::OnGetNextNonceForHardware,
                             weak_factory_.GetWeakPtr(), std::move(meta),
                             std::move(callback)));
  } else {
    uint256_t nonce = meta->tx->nonce().value();
    OnGetNextNonceForHardware(std::move(meta), std::move(callback), true,
                              nonce);
  }
}

void EthTxController::GetTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run("");
    return;
  }
  uint256_t chain_id = 0;
  if (!HexValueToUint256(rpc_controller_->GetChainId(), &chain_id)) {
    std::move(callback).Run("");
    return;
  }
  auto message = meta->tx->GetMessageToSign(chain_id, false);
  auto encoded = brave_wallet::ToHex(message);
  std::move(callback).Run(encoded);
}

void EthTxController::GetTransactionInfo(const std::string& tx_meta_id,
                                         GetTransactionInfoCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(EthTxStateManager::TxMetaToTransactionInfo(*meta));
}

void EthTxController::OnGetNextNonceForHardware(
    std::unique_ptr<EthTxStateManager::TxMeta> meta,
    ApproveHardwareTransactionCallback callback,
    bool success,
    uint256_t nonce) {
  if (!success) {
    meta->status = mojom::TransactionStatus::Error;
    tx_state_manager_->AddOrUpdateTx(*meta);
    LOG(ERROR) << "GetNextNonce failed";
    std::move(callback).Run(false);
    return;
  }
  meta->tx->set_nonce(nonce);
  meta->status = mojom::TransactionStatus::Approved;
  tx_state_manager_->AddOrUpdateTx(*meta);
  std::move(callback).Run(true);
}

void EthTxController::ProcessHardwareSignature(
    const std::string& tx_meta_id,
    const std::string& v,
    const std::string& r,
    const std::string& s,
    ProcessHardwareSignatureCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(false);
    return;
  }
  if (!meta->tx->ProcessVRS(v, r, s)) {
    LOG(ERROR) << "Could not initialize a transaction with v,r,s";
    meta->status = mojom::TransactionStatus::Error;
    tx_state_manager_->AddOrUpdateTx(*meta);
    std::move(callback).Run(false);
    return;
  }
  auto data = meta->tx->GetSignedTransaction();
  PublishTransaction(tx_meta_id, data);
  std::move(callback).Run(true);
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

  if (!meta->tx->nonce()) {
    auto from = EthAddress(meta->from);
    nonce_tracker_->GetNextNonce(
        from,
        base::BindOnce(&EthTxController::OnGetNextNonce,
                       weak_factory_.GetWeakPtr(), std::move(meta), chain_id));
  } else {
    uint256_t nonce = meta->tx->nonce().value();
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
    LOG(ERROR) << "GetNextNonce failed";
    return;
  }
  meta->tx->set_nonce(nonce);
  DCHECK(!keyring_controller_->IsLocked());
  keyring_controller_->SignTransactionByDefaultKeyring(
      meta->from.ToChecksumAddress(), meta->tx.get(), chain_id);
  meta->status = mojom::TransactionStatus::Approved;
  tx_state_manager_->AddOrUpdateTx(*meta);
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

  if (status) {
    UpdatePendingTransactions();
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

void EthTxController::MakeERC721TransferFromData(
    const std::string& from,
    const std::string& to,
    const std::string& token_id,
    const std::string& contract_address,
    MakeERC721TransferFromDataCallback callback) {
  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    VLOG(1) << __FUNCTION__ << ": Could not convert token_id";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  // Check if safeTransferFrom is supported first.
  rpc_controller_->GetSupportsInterface(
      contract_address, kERC721InterfaceId,
      base::BindOnce(&EthTxController::ContinueMakeERC721TransferFromData,
                     weak_factory_.GetWeakPtr(), from, to, token_id_uint,
                     std::move(callback)));
}

void EthTxController::ContinueMakeERC721TransferFromData(
    const std::string& from,
    const std::string& to,
    uint256_t token_id,
    MakeERC721TransferFromDataCallback callback,
    bool success,
    bool is_safe_transfer_from_supported) {
  std::string data;
  if (!erc721::TransferFromOrSafeTransferFrom(is_safe_transfer_from_supported,
                                              from, to, token_id, &data)) {
    VLOG(1) << __FUNCTION__
            << ": Could not make transferFrom/safeTransferFrom data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  if (!base::StartsWith(data, "0x")) {
    VLOG(1) << __FUNCTION__
            << ": Invalid format returned from TransferFromOrSafeTransferFrom";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::vector<uint8_t> data_decoded;
  if (!base::HexStringToBytes(data.data() + 2, &data_decoded)) {
    VLOG(1) << __FUNCTION__ << ": Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxController::AddObserver(
    ::mojo::PendingRemote<mojom::EthTxControllerObserver> observer) {
  observers_.Add(std::move(observer));
}

void EthTxController::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_)
    observer->OnTransactionStatusChanged(tx_info->Clone());
}

void EthTxController::OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_)
    observer->OnNewUnapprovedTx(tx_info->Clone());
}

void EthTxController::NotifyUnapprovedTxUpdated(
    EthTxStateManager::TxMeta* meta) {
  for (const auto& observer : observers_)
    observer->OnUnapprovedTxUpdated(
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

void EthTxController::SetGasPriceAndLimitForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::string& gas_price,
    const std::string& gas_limit,
    SetGasPriceAndLimitForUnapprovedTransactionCallback callback) {
  if (gas_price.empty() || gas_limit.empty()) {
    std::move(callback).Run(false);
    return;
  }

  std::unique_ptr<EthTxStateManager::TxMeta> tx_meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!tx_meta || tx_meta->status != mojom::TransactionStatus::Unapproved) {
    std::move(callback).Run(false);
    return;
  }

  uint256_t value;
  if (!HexValueToUint256(gas_price, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx_meta->tx->set_gas_price(value);

  if (!HexValueToUint256(gas_limit, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx_meta->tx->set_gas_limit(value);

  tx_state_manager_->AddOrUpdateTx(*tx_meta);
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

void EthTxController::SetGasFeeAndLimitForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::string& max_priority_fee_per_gas,
    const std::string& max_fee_per_gas,
    const std::string& gas_limit,
    SetGasFeeAndLimitForUnapprovedTransactionCallback callback) {
  if (max_priority_fee_per_gas.empty() || max_fee_per_gas.empty() ||
      gas_limit.empty()) {
    std::move(callback).Run(false);
    return;
  }

  std::unique_ptr<EthTxStateManager::TxMeta> tx_meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!tx_meta || tx_meta->status != mojom::TransactionStatus::Unapproved ||
      tx_meta->tx->type() != 2 /* Eip1559 */) {
    std::move(callback).Run(false);
    return;
  }

  auto* tx1559 = reinterpret_cast<Eip1559Transaction*>(tx_meta->tx.get());

  uint256_t value;
  if (!HexValueToUint256(max_priority_fee_per_gas, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx1559->set_max_priority_fee_per_gas(value);

  if (!HexValueToUint256(max_fee_per_gas, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx1559->set_max_fee_per_gas(value);

  if (!HexValueToUint256(gas_limit, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx1559->set_gas_limit(value);

  tx_state_manager_->AddOrUpdateTx(*tx_meta);
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

void EthTxController::SetDataForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::vector<uint8_t>& data,
    SetDataForUnapprovedTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> tx_meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!tx_meta || tx_meta->status != mojom::TransactionStatus::Unapproved) {
    std::move(callback).Run(false);
    return;
  }

  tx_meta->tx->set_data(data);
  tx_state_manager_->AddOrUpdateTx(*tx_meta);
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

std::unique_ptr<EthTxStateManager::TxMeta> EthTxController::GetTxForTesting(
    const std::string& tx_meta_id) {
  return tx_state_manager_->GetTx(tx_meta_id);
}

void EthTxController::CheckIfBlockTrackerShouldRun() {
  bool locked = keyring_controller_->IsLocked();
  bool running = eth_block_tracker_->IsRunning();
  if (!locked && !running) {
    eth_block_tracker_->Start(
        base::TimeDelta::FromSeconds(kBlockTrackerDefaultTimeInSeconds));
  } else if ((locked || known_no_pending_tx) && running) {
    eth_block_tracker_->Stop();
  }
}

void EthTxController::OnNewBlock(uint256_t block_num) {
  UpdatePendingTransactions();
}

void EthTxController::UpdatePendingTransactions() {
  size_t num_pending;
  if (pending_tx_tracker_->UpdatePendingTransactions(&num_pending)) {
    known_no_pending_tx = num_pending == 0;
    if (known_no_pending_tx) {
      CheckIfBlockTrackerShouldRun();
    }
  }
}

void EthTxController::Locked() {
  CheckIfBlockTrackerShouldRun();
}

void EthTxController::Unlocked() {
  CheckIfBlockTrackerShouldRun();
  UpdatePendingTransactions();
}

void EthTxController::KeyringCreated() {
  UpdatePendingTransactions();
}

void EthTxController::KeyringRestored() {
  UpdatePendingTransactions();
}

void EthTxController::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta || meta->status != mojom::TransactionStatus::Submitted) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  if (meta->tx->type() == 2) {  // EIP1559
    auto tx = std::make_unique<Eip1559Transaction>(
        *reinterpret_cast<Eip1559Transaction*>(meta->tx.get()));
    if (cancel) {
      tx->set_to(meta->from);
      tx->set_value(0);
      tx->set_data(std::vector<uint8_t>());
    }

    asset_ratio_controller_->GetGasOracle(base::BindOnce(
        &EthTxController::ContinueSpeedupOrCancel1559Transaction,
        weak_factory_.GetWeakPtr(), meta->from.ToChecksumAddress(),
        Uint256ValueToHex(meta->tx->gas_limit()), std::move(tx),
        std::move(callback)));
  } else {
    auto tx = std::make_unique<EthTransaction>(*meta->tx);
    if (cancel) {
      tx->set_to(meta->from);
      tx->set_value(0);
      tx->set_data(std::vector<uint8_t>());
    }

    mojom::TransactionType tx_type;
    if (!GetTransactionInfoFromData(ToHex(tx->data()), &tx_type, nullptr,
                                    nullptr)) {
      std::move(callback).Run(
          false, "",
          l10n_util::GetStringUTF8(
              IDS_WALLET_ETH_SEND_TRANSACTION_GET_TX_TYPE_FAILED));
      return;
    }

    if (tx_type == mojom::TransactionType::ETHSend) {
      ContinueSpeedupOrCancelTransaction(
          meta->from.ToChecksumAddress(),
          Uint256ValueToHex(meta->tx->gas_limit()), std::move(tx),
          std::move(callback), true,
          Uint256ValueToHex(kDefaultSendEthGasPrice));
    } else {
      rpc_controller_->GetGasPrice(base::BindOnce(
          &EthTxController::ContinueSpeedupOrCancelTransaction,
          weak_factory_.GetWeakPtr(), meta->from.ToChecksumAddress(),
          Uint256ValueToHex(meta->tx->gas_limit()), std::move(tx),
          std::move(callback)));
    }
  }
}

void EthTxController::ContinueSpeedupOrCancelTransaction(
    const std::string& from,
    const std::string& gas_limit,
    std::unique_ptr<EthTransaction> tx,
    SpeedupOrCancelTransactionCallback callback,
    bool success,
    const std::string& result) {
  uint256_t latest_estimate_gas_price;
  if (!success || !HexValueToUint256(result, &latest_estimate_gas_price)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_PRICE_FAILED));
    return;
  }

  // Update gas price to max(latest_estimate, original_gas_price + 10%).
  // Original_gas_price * 11 / 10 is done using uint64_t because uint256_t does
  // not support division. It's fairly safe to do so because it's unlikely the
  // gas value will be larger than that, gas value is usually around 10^12 wei.
  if (tx->gas_price() > std::numeric_limits<uint64_t>::max()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_PRICE_FAILED));
    return;
  }

  uint256_t increased_gas_price =
      static_cast<uint64_t>(tx->gas_price()) * 11ULL / 10ULL;
  tx->set_gas_price(std::max(latest_estimate_gas_price, increased_gas_price));

  ContinueAddUnapprovedTransaction(from, std::move(tx), std::move(callback),
                                   true, gas_limit);
}

void EthTxController::ContinueSpeedupOrCancel1559Transaction(
    const std::string& from,
    const std::string& gas_limit,
    std::unique_ptr<Eip1559Transaction> tx,
    SpeedupOrCancelTransactionCallback callback,
    mojom::GasEstimation1559Ptr gas_estimation) {
  auto estimation =
      Eip1559Transaction::GasEstimation::FromMojomGasEstimation1559(
          std::move(gas_estimation));
  if (!estimation) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_FEES_FAILED));
    return;
  }

  // Update gas fees to max(latest_estimate, original_gas_fee + 10%).
  // Original_gas_fee * 11 / 10 is done using uint64_t because uint256_t does
  // not support division. It's fairly safe to do so because it's unlikely the
  // gas fees will be larger than that, they are usually around 10^12 wei.
  if (tx->max_priority_fee_per_gas() > std::numeric_limits<uint64_t>::max() ||
      tx->max_fee_per_gas() > std::numeric_limits<uint64_t>::max()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_FEES_FAILED));
    return;
  }

  uint256_t increased_max_priority_fee_per_gas =
      static_cast<uint64_t>(tx->max_priority_fee_per_gas()) * 11ULL / 10ULL;
  uint256_t increased_max_fee_per_gas =
      static_cast<uint64_t>(tx->max_fee_per_gas()) * 11ULL / 10ULL;
  tx->set_max_fee_per_gas(
      std::max(estimation->avg_max_fee_per_gas, increased_max_fee_per_gas));
  tx->set_max_priority_fee_per_gas(
      std::max(estimation->avg_max_priority_fee_per_gas,
               increased_max_priority_fee_per_gas));

  ContinueAddUnapprovedTransaction(from, std::move(tx), std::move(callback),
                                   true, gas_limit);
}

void EthTxController::RetryTransaction(const std::string& tx_meta_id,
                                       RetryTransactionCallback callback) {
  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(tx_meta_id);
  if (!meta || meta->status != mojom::TransactionStatus::Error) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  std::unique_ptr<EthTransaction> tx;
  if (meta->tx->type() == 2) {  // EIP1559
    tx = std::make_unique<Eip1559Transaction>(
        *reinterpret_cast<Eip1559Transaction*>(meta->tx.get()));
  } else {
    tx = std::make_unique<EthTransaction>(*meta->tx);
  }

  ContinueAddUnapprovedTransaction(meta->from.ToChecksumAddress(),
                                   std::move(tx), std::move(callback), true,
                                   Uint256ValueToHex(meta->tx->gas_limit()));
}

}  // namespace brave_wallet
