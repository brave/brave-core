/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_tx_manager.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eip1559_transaction.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/browser/eth_gas_utils.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

// static
bool EthTxManager::ValidateTxData(const mojom::TxDataPtr& tx_data,
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
    *error = l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_TO_INVALID);
    return false;
  }
  return true;
}

// static
bool EthTxManager::ValidateTxData1559(const mojom::TxData1559Ptr& tx_data,
                                      std::string* error) {
  if (!ValidateTxData(tx_data->base_data, error)) {
    return false;
  }
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

EthTxManager::EthTxManager(TxService& tx_service,
                           JsonRpcService* json_rpc_service,
                           KeyringService& keyring_service,
                           TxStorageDelegate& delegate,
                           AccountResolverDelegate& account_resolver_delegate)
    : TxManager(std::make_unique<EthTxStateManager>(delegate,
                                                    account_resolver_delegate),
                std::make_unique<EthBlockTracker>(json_rpc_service),
                tx_service,
                keyring_service),
      nonce_tracker_(std::make_unique<EthNonceTracker>(&GetEthTxStateManager(),
                                                       json_rpc_service)),
      pending_tx_tracker_(
          std::make_unique<EthPendingTxTracker>(&GetEthTxStateManager(),
                                                json_rpc_service,
                                                nonce_tracker_.get())),
      json_rpc_service_(json_rpc_service) {
  GetEthBlockTracker().AddObserver(this);
}

EthTxManager::~EthTxManager() {
  GetEthBlockTracker().RemoveObserver(this);
}

void EthTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  DCHECK(tx_data_union->is_eth_tx_data() ||
         tx_data_union->is_eth_tx_data_1559());
  auto origin_val =
      origin.value_or(url::Origin::Create(GURL("chrome://wallet")));
  if (tx_data_union->is_eth_tx_data()) {
    AddUnapprovedTransaction(chain_id,
                             std::move(tx_data_union->get_eth_tx_data()), from,
                             std::move(origin_val), std::move(callback));
  } else {
    AddUnapproved1559Transaction(
        chain_id, std::move(tx_data_union->get_eth_tx_data_1559()), from,
        std::move(origin_val), std::move(callback));
  }
}

void EthTxManager::AddUnapprovedEvmTransaction(
    mojom::NewEvmTransactionParamsPtr params,
    const std::optional<url::Origin>& origin,
    AddUnapprovedEvmTransactionCallback callback) {
  auto origin_val =
      origin.value_or(url::Origin::Create(GURL("chrome://wallet")));

  auto tx_data =
      mojom::TxData::New("", "", params->gas_limit, params->to, params->value,
                         params->data, false, std::nullopt);

  if (!json_rpc_service_->network_manager()
           ->IsEip1559Chain(params->chain_id)
           .value_or(false)) {
    AddUnapprovedTransaction(params->chain_id, std::move(tx_data), params->from,
                             std::move(origin_val), std::move(callback));
  } else {
    auto tx_data_1559 = mojom::TxData1559::New(
        std::move(tx_data), params->chain_id, "", "", nullptr);
    AddUnapproved1559Transaction(params->chain_id, std::move(tx_data_1559),
                                 params->from, std::move(origin_val),
                                 std::move(callback));
  }
}

void EthTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataPtr tx_data,
    const mojom::AccountIdPtr& from,
    const url::Origin& origin,
    AddUnapprovedTransactionCallback callback) {
  std::string error;
  if (!EthTxManager::ValidateTxData(tx_data, &error)) {
    std::move(callback).Run(false, "", error);
    return;
  }
  auto tx = EthTransaction::FromTxData(tx_data, false);
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }

  auto tx_ptr = std::make_unique<EthTransaction>(*tx);
  const std::string gas_limit = Uint256ValueToHex(tx_ptr->gas_limit());

  // Use empty string for data to estimate gas when data array is empty,
  // as required by geth. This is typically the case with ETHSend.
  const std::string data = tx_data->data.empty() ? "" : ToHex(tx_data->data);

  if (!tx_ptr->gas_price()) {
    json_rpc_service_->GetGasPrice(
        chain_id,
        base::BindOnce(&EthTxManager::OnGetGasPrice, weak_factory_.GetWeakPtr(),
                       chain_id, from.Clone(), origin, tx_data->to,
                       tx_data->value, data, gas_limit, std::move(tx_ptr),
                       std::move(callback), tx_data->sign_only));
  } else if (!tx_ptr->gas_limit()) {
    json_rpc_service_->GetEstimateGas(
        chain_id, from->address, tx_data->to, "" /* gas */, "" /* gas_price */,
        tx_data->value, data,
        base::BindOnce(&EthTxManager::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                       origin, std::move(tx_ptr), std::move(callback),
                       tx_data->sign_only));
  } else {
    ContinueAddUnapprovedTransaction(
        chain_id, from, origin, std::move(tx_ptr), std::move(callback),
        tx_data->sign_only, gas_limit, mojom::ProviderError::kSuccess, "");
  }
}

void EthTxManager::OnGetGasPrice(const std::string& chain_id,
                                 const mojom::AccountIdPtr& from,
                                 const url::Origin& origin,
                                 const std::string& to,
                                 const std::string& value,
                                 const std::string& data,
                                 const std::string& gas_limit,
                                 std::unique_ptr<EthTransaction> tx,
                                 AddUnapprovedTransactionCallback callback,
                                 bool sign_only,
                                 const std::string& result,
                                 mojom::ProviderError error,
                                 const std::string& error_message) {
  uint256_t gas_price;
  if (error != mojom::ProviderError::kSuccess ||
      !HexValueToUint256(result, &gas_price)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_GET_GAS_PRICE_FAILED));
    return;
  }
  tx->set_gas_price(gas_price);

  if (!tx->gas_limit()) {
    json_rpc_service_->GetEstimateGas(
        chain_id, from->address, to, "" /* gas */, "" /* gas_price */, value,
        data,
        base::BindOnce(&EthTxManager::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                       origin, std::move(tx), std::move(callback), sign_only));
  } else {
    ContinueAddUnapprovedTransaction(chain_id, from, origin, std::move(tx),
                                     std::move(callback), sign_only, gas_limit,
                                     mojom::ProviderError::kSuccess, "");
  }
}

void EthTxManager::ContinueAddUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    std::unique_ptr<EthTransaction> tx,
    AddUnapprovedTransactionCallback callback,
    bool sign_only,
    const std::string& result,
    mojom::ProviderError error,
    const std::string& error_message) {
  uint256_t gas_limit;
  if (error != mojom::ProviderError::kSuccess ||
      !HexValueToUint256(result, &gas_limit)) {
    gas_limit = 0;
    auto tx_info = GetTransactionInfoFromData(tx->data());
    if (tx_info) {
      mojom::TransactionType tx_type = std::get<0>(*tx_info);

      // Try to use reasonable values when we can't get an estimation.
      // These are taken via looking through the different types of transactions
      // on etherscan and taking the next rounded up value for the largest found
      if (tx_type == mojom::TransactionType::ETHSend ||
          tx_type == mojom::TransactionType::ETHFilForwarderTransfer) {
        gas_limit = kDefaultSendEthGasLimit;
      } else if (tx_type == mojom::TransactionType::ERC20Transfer) {
        gas_limit = kDefaultERC20TransferGasLimit;
      } else if (tx_type == mojom::TransactionType::ERC721TransferFrom ||
                 tx_type == mojom::TransactionType::ERC721SafeTransferFrom) {
        gas_limit = kDefaultERC721TransferGasLimit;
      } else if (tx_type == mojom::TransactionType::ERC20Approve) {
        gas_limit = kDefaultERC20ApproveGasLimit;
      }
    }
  }
  tx->set_gas_limit(gas_limit);

  EthTxMeta meta(from, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_origin(origin);
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_sign_only(sign_only);
  meta.set_chain_id(chain_id);
  if (!tx_state_manager().AddOrUpdateTx(meta)) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void EthTxManager::AddUnapproved1559Transaction(
    const std::string& chain_id,
    mojom::TxData1559Ptr tx_data,
    const mojom::AccountIdPtr& from,
    const url::Origin& origin,
    AddUnapprovedTransactionCallback callback) {
  std::string error;
  if (!EthTxManager::ValidateTxData1559(tx_data, &error)) {
    std::move(callback).Run(false, "", error);
    return;
  }
  auto tx = Eip1559Transaction::FromTxData(tx_data, false);
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }

  auto tx_ptr = std::make_unique<Eip1559Transaction>(*tx);
  std::string gas_limit = tx_data->base_data->gas_limit;

  // Use empty string for data to estimate gas when data array is empty,
  // as required by geth. This is typically the case with ETHSend.
  const std::string data =
      tx_data->base_data->data.empty() ? "" : ToHex(tx_data->base_data->data);
  bool sign_only = tx_data->base_data->sign_only;

  if (!tx_ptr->max_priority_fee_per_gas() || !tx_ptr->max_fee_per_gas()) {
    GetGasEstimation1559(
        chain_id,
        base::BindOnce(&EthTxManager::OnGetGasOracleForUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                       origin, tx_data->base_data->to,
                       tx_data->base_data->value, data, gas_limit,
                       std::move(tx_ptr), std::move(callback), sign_only));
  } else if (gas_limit.empty()) {
    json_rpc_service_->GetEstimateGas(
        chain_id, from->address, tx_data->base_data->to, "" /* gas */,
        "" /* gas_price */, tx_data->base_data->value, data,
        base::BindOnce(&EthTxManager::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                       origin, std::move(tx_ptr), std::move(callback),
                       sign_only));
  } else {
    ContinueAddUnapprovedTransaction(chain_id, from, origin, std::move(tx_ptr),
                                     std::move(callback), sign_only, gas_limit,
                                     mojom::ProviderError::kSuccess, "");
  }
}

void EthTxManager::OnGetGasOracleForUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const url::Origin& origin,
    const std::string& to,
    const std::string& value,
    const std::string& data,
    const std::string& gas_limit,
    std::unique_ptr<Eip1559Transaction> tx,
    AddUnapprovedTransactionCallback callback,
    bool sign_only,
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
    json_rpc_service_->GetEstimateGas(
        chain_id, from->address, to, "" /* gas */, "" /* gas_price */, value,
        data,
        base::BindOnce(&EthTxManager::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                       origin, std::move(tx), std::move(callback), sign_only));
  } else {
    ContinueAddUnapprovedTransaction(chain_id, from, origin, std::move(tx),
                                     std::move(callback), sign_only, gas_limit,
                                     mojom::ProviderError::kSuccess, "");
  }
}

void EthTxManager::GetNonceForHardwareTransaction(
    const std::string& tx_meta_id,
    GetNonceForHardwareTransactionCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(std::nullopt);
    return;
  }
  if (!meta->tx()->nonce()) {
    auto from = meta->from().Clone();
    auto chain_id = meta->chain_id();
    nonce_tracker_->GetNextNonce(
        chain_id, from,
        base::BindOnce(&EthTxManager::OnGetNextNonceForHardware,
                       weak_factory_.GetWeakPtr(), std::move(meta),
                       std::move(callback)));
  } else {
    uint256_t nonce = meta->tx()->nonce().value();
    OnGetNextNonceForHardware(std::move(meta), std::move(callback), true,
                              nonce);
  }
}

void EthTxManager::GetEthTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetEthTransactionMessageToSignCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!meta) {
    VLOG(1) << __FUNCTION__ << "No transaction found with id:" << tx_meta_id;
    std::move(callback).Run(std::nullopt);
    return;
  }
  uint256_t chain_id = 0;
  if (!HexValueToUint256(meta->chain_id(), &chain_id)) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(
      HexEncodeLower(meta->tx()->GetMessageToSign(chain_id)));
}

mojom::CoinType EthTxManager::GetCoinType() const {
  return mojom::CoinType::ETH;
}

void EthTxManager::OnGetNextNonceForHardware(
    std::unique_ptr<EthTxMeta> meta,
    GetNonceForHardwareTransactionCallback callback,
    bool success,
    uint256_t nonce) {
  if (!success) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager().AddOrUpdateTx(*meta);
    VLOG(1) << __FUNCTION__
            << "GetNextNonce failed for tx with meta:" << meta->id();
    std::move(callback).Run(std::nullopt);
    return;
  }
  meta->tx()->set_nonce(nonce);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  std::move(callback).Run(Uint256ValueToHex(nonce));
}

void EthTxManager::ProcessEthHardwareSignature(
    const std::string& tx_meta_id,
    mojom::EthereumSignatureVRSPtr hw_signature,
    ProcessEthHardwareSignatureCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!meta) {
    VLOG(1) << __FUNCTION__ << "No transaction found with id" << tx_meta_id;
    std::move(callback).Run(
        false, mojom::ProviderError::kResourceNotFound,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }
  if (!meta->tx()->ProcessVRS(hw_signature->v_bytes, hw_signature->r_bytes,
                              hw_signature->s_bytes)) {
    VLOG(1) << __FUNCTION__
            << "Could not initialize a transaction with v,r,s for id:"
            << tx_meta_id;
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager().AddOrUpdateTx(*meta);
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(
            IDS_BRAVE_WALLET_HARDWARE_PROCESS_TRANSACTION_ERROR));
    return;
  }
  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto data = meta->tx()->GetSignedTransaction();

  auto internal_callback =
      base::BindOnce(&EthTxManager::ContinueProcessHardwareSignature,
                     weak_factory_.GetWeakPtr(), std::move(callback));

  PublishTransaction(meta->chain_id(), tx_meta_id, data,
                     std::move(internal_callback));
}

void EthTxManager::ContinueProcessHardwareSignature(
    ProcessEthHardwareSignatureCallback callback,
    bool status,
    mojom::ProviderErrorUnionPtr error_union,
    const std::string& error_message) {
  DCHECK(error_union && error_union->is_provider_error());
  std::move(callback).Run(status, error_union->get_provider_error(),
                          error_message);
}

void EthTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kResourceNotFound),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  if (!meta->tx()->nonce()) {
    auto from = meta->from().Clone();
    auto chain_id = meta->chain_id();
    nonce_tracker_->GetNextNonce(
        chain_id, from,
        base::BindOnce(&EthTxManager::OnGetNextNonce,
                       weak_factory_.GetWeakPtr(), std::move(meta),
                       std::move(callback)));
  } else {
    uint256_t nonce = meta->tx()->nonce().value();
    OnGetNextNonce(std::move(meta), std::move(callback), true, nonce);
  }
}

void EthTxManager::OnGetNextNonce(std::unique_ptr<EthTxMeta> meta,
                                  ApproveTransactionCallback callback,
                                  bool success,
                                  uint256_t nonce) {
  if (!success) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager().AddOrUpdateTx(*meta);
    LOG(ERROR) << "GetNextNonce failed";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_GET_NONCE_ERROR));
    return;
  }

  uint256_t chain_id = 0;
  if (!HexValueToUint256(meta->chain_id(), &chain_id)) {
    LOG(ERROR) << "Could not convert chain ID";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_INVALID_CHAIN_ID_RPC));
    return;
  }

  meta->tx()->set_nonce(nonce);

  if (keyring_service().IsLockedSync()) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  keyring_service().SignTransactionByDefaultKeyring(*meta->from(), meta->tx(),
                                                    chain_id);
  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  if (!meta->tx()->IsSigned()) {
    LOG(ERROR) << "Transaction must be signed first";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_SIGN_TRANSACTION_ERROR));
    return;
  }
  if (meta->sign_only()) {
    meta->set_status(mojom::TransactionStatus::Signed);
    meta->set_tx_hash(meta->tx()->GetTransactionHash());
    if (!tx_state_manager().AddOrUpdateTx(*meta)) {
      std::move(callback).Run(
          false,
          mojom::ProviderErrorUnion::NewProviderError(
              mojom::ProviderError::kInternalError),
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }

    std::move(callback).Run(true,
                            mojom::ProviderErrorUnion::NewProviderError(
                                mojom::ProviderError::kSuccess),
                            "");
    UpdatePendingTransactions(meta->chain_id());
  } else {
    PublishTransaction(meta->chain_id(), meta->id(),
                       meta->tx()->GetSignedTransaction(), std::move(callback));
  }
}

void EthTxManager::PublishTransaction(const std::string& chain_id,
                                      const std::string& tx_meta_id,
                                      const std::string& signed_transaction,
                                      ApproveTransactionCallback callback) {
  json_rpc_service_->SendRawTransaction(
      chain_id, signed_transaction,
      base::BindOnce(&EthTxManager::OnPublishTransaction,
                     weak_factory_.GetWeakPtr(), chain_id, tx_meta_id,
                     std::move(callback)));
}

void EthTxManager::OnPublishTransaction(const std::string& chain_id,
                                        const std::string& tx_meta_id,
                                        ApproveTransactionCallback callback,
                                        const std::string& tx_hash,
                                        mojom::ProviderError error,
                                        const std::string& error_message) {
  std::unique_ptr<TxMeta> meta = tx_state_manager().GetTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kResourceNotFound),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  if (error == mojom::ProviderError::kSuccess) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_hash);
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewProviderError(
            mojom::ProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  if (error == mojom::ProviderError::kSuccess) {
    UpdatePendingTransactions(chain_id);
  }

  std::move(callback).Run(error_message.empty(),
                          mojom::ProviderErrorUnion::NewProviderError(error),
                          error_message);
}

void EthTxManager::MakeFilForwarderTransferData(
    const FilAddress& fil_address,
    MakeFilForwarderDataCallback callback) {
  std::optional<std::vector<uint8_t>> data = filforwarder::Forward(fil_address);

  if (!data) {
    LOG(ERROR) << "Could not make transfer data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data.value());
}

void EthTxManager::MakeERC20TransferData(
    const std::string& to_address,
    const std::string& amount,
    MakeERC20TransferDataCallback callback) {
  if (BlockchainRegistry::GetInstance()->IsOfacAddress(to_address)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

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

  std::vector<uint8_t> data_decoded;
  if (!PrefixedHexStringToBytes(data, &data_decoded)) {
    LOG(ERROR) << "Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxManager::MakeERC20ApproveData(const std::string& spender_address,
                                        const std::string& amount,
                                        MakeERC20ApproveDataCallback callback) {
  if (BlockchainRegistry::GetInstance()->IsOfacAddress(spender_address)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

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

  std::vector<uint8_t> data_decoded;
  if (!PrefixedHexStringToBytes(data.data(), &data_decoded)) {
    LOG(ERROR) << "Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxManager::MakeERC721TransferFromData(
    const std::string& from,
    const std::string& to,
    const std::string& token_id,
    const std::string& contract_address,
    MakeERC721TransferFromDataCallback callback) {
  if (BlockchainRegistry::GetInstance()->IsOfacAddress(to)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    VLOG(1) << __FUNCTION__ << ": Could not convert token_id";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  const std::string chain_id =
      json_rpc_service_->GetChainIdSync(mojom::CoinType::ETH, std::nullopt);
  // Check if safeTransferFrom is supported first.
  json_rpc_service_->GetSupportsInterface(
      contract_address, kERC721InterfaceId, chain_id,
      base::BindOnce(&EthTxManager::ContinueMakeERC721TransferFromData,
                     weak_factory_.GetWeakPtr(), from, to, token_id_uint,
                     std::move(callback)));
}

void EthTxManager::ContinueMakeERC721TransferFromData(
    const std::string& from,
    const std::string& to,
    uint256_t token_id,
    MakeERC721TransferFromDataCallback callback,
    bool is_safe_transfer_from_supported,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::string data;
  if (!erc721::TransferFromOrSafeTransferFrom(is_safe_transfer_from_supported,
                                              from, to, token_id, &data)) {
    VLOG(1) << __FUNCTION__
            << ": Could not make transferFrom/safeTransferFrom data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::vector<uint8_t> data_decoded;
  if (!PrefixedHexStringToBytes(data, &data_decoded)) {
    VLOG(1) << __FUNCTION__ << ": Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxManager::MakeERC1155TransferFromData(
    const std::string& from,
    const std::string& to,
    const std::string& token_id,
    const std::string& value,
    const std::string& contract_address,
    MakeERC1155TransferFromDataCallback callback) {
  if (BlockchainRegistry::GetInstance()->IsOfacAddress(to)) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    VLOG(1) << __FUNCTION__ << ": Could not convert token_id";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  uint256_t value_uint = 0;
  if (!HexValueToUint256(value, &value_uint) || (value_uint == 0)) {
    VLOG(1) << __FUNCTION__ << ": Could not convert value";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::string data;
  if (!erc1155::SafeTransferFrom(from, to, token_id_uint, value_uint, &data)) {
    VLOG(1) << __FUNCTION__ << ": Could not make safeTransferFrom data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::vector<uint8_t> data_decoded;
  if (!PrefixedHexStringToBytes(data, &data_decoded)) {
    VLOG(1) << __FUNCTION__ << ": Could not decode data";
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }

  std::move(callback).Run(true, data_decoded);
}

void EthTxManager::NotifyUnapprovedTxUpdated(TxMeta* meta) {
  tx_service().OnUnapprovedTxUpdated(meta->ToTransactionInfo());
}

void EthTxManager::SetGasPriceAndLimitForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::string& gas_price,
    const std::string& gas_limit,
    SetGasPriceAndLimitForUnapprovedTransactionCallback callback) {
  if (gas_price.empty() || gas_limit.empty()) {
    std::move(callback).Run(false);
    return;
  }

  auto tx_meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!tx_meta || tx_meta->status() != mojom::TransactionStatus::Unapproved) {
    std::move(callback).Run(false);
    return;
  }

  uint256_t value;
  if (!HexValueToUint256(gas_price, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx_meta->tx()->set_gas_price(value);

  if (!HexValueToUint256(gas_limit, &value)) {
    std::move(callback).Run(false);
    return;
  }
  tx_meta->tx()->set_gas_limit(value);

  if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
    std::move(callback).Run(false);
    return;
  }
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

void EthTxManager::SetGasFeeAndLimitForUnapprovedTransaction(
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

  auto tx_meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!tx_meta || tx_meta->status() != mojom::TransactionStatus::Unapproved ||
      tx_meta->tx()->type() != 2 /* Eip1559 */) {
    std::move(callback).Run(false);
    return;
  }

  auto* tx1559 = static_cast<Eip1559Transaction*>(tx_meta->tx());

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

  if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
    std::move(callback).Run(false);
    return;
  }
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

void EthTxManager::SetDataForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::vector<uint8_t>& data,
    SetDataForUnapprovedTransactionCallback callback) {
  auto tx_meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!tx_meta || tx_meta->status() != mojom::TransactionStatus::Unapproved) {
    std::move(callback).Run(false);
    return;
  }

  tx_meta->tx()->set_data(data);
  if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
    std::move(callback).Run(false);
    return;
  }
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

void EthTxManager::SetNonceForUnapprovedTransaction(
    const std::string& tx_meta_id,
    const std::string& nonce,
    SetNonceForUnapprovedTransactionCallback callback) {
  auto tx_meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!tx_meta || tx_meta->status() != mojom::TransactionStatus::Unapproved) {
    std::move(callback).Run(false);
    return;
  }

  if (nonce.empty()) {
    tx_meta->tx()->set_nonce(std::nullopt);
  } else {
    uint256_t nonce_uint;
    if (!HexValueToUint256(nonce, &nonce_uint)) {
      std::move(callback).Run(false);
      return;
    }
    tx_meta->tx()->set_nonce(nonce_uint);
  }
  if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
    std::move(callback).Run(false);
    return;
  }
  NotifyUnapprovedTxUpdated(tx_meta.get());
  std::move(callback).Run(true);
}

std::unique_ptr<EthTxMeta> EthTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetEthTxStateManager().GetEthTx(tx_meta_id);
}

void EthTxManager::OnNewBlock(const std::string& chain_id,
                              uint256_t block_num) {
  UpdatePendingTransactions(chain_id);
}

void EthTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  std::set<std::string> pending_chain_ids;
  if (pending_tx_tracker_->UpdatePendingTransactions(chain_id,
                                                     &pending_chain_ids)) {
    CheckIfBlockTrackerShouldRun(pending_chain_ids);
  }
}

void EthTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);
  if (!meta || meta->status() != mojom::TransactionStatus::Submitted) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  if (meta->tx()->type() == 2) {  // EIP1559
    auto tx = std::make_unique<Eip1559Transaction>(
        *static_cast<Eip1559Transaction*>(meta->tx()));
    if (cancel) {
      tx->set_to(EthAddress::FromHex(meta->from()->address));
      tx->set_value(0);
      tx->set_data(std::vector<uint8_t>());
    }

    GetGasEstimation1559(
        meta->chain_id(),
        base::BindOnce(&EthTxManager::ContinueSpeedupOrCancel1559Transaction,
                       weak_factory_.GetWeakPtr(), meta->chain_id(),
                       meta->from().Clone(), meta->origin(),
                       Uint256ValueToHex(meta->tx()->gas_limit()),
                       std::move(tx), std::move(callback)));
  } else {
    auto tx = std::make_unique<EthTransaction>(*meta->tx());
    if (cancel) {
      tx->set_to(EthAddress::FromHex(meta->from()->address));
      tx->set_value(0);
      tx->set_data(std::vector<uint8_t>());
    }

    if (!GetTransactionInfoFromData(tx->data())) {
      std::move(callback).Run(
          false, "",
          l10n_util::GetStringUTF8(
              IDS_WALLET_ETH_SEND_TRANSACTION_GET_TX_TYPE_FAILED));
      return;
    }

    json_rpc_service_->GetGasPrice(
        meta->chain_id(),
        base::BindOnce(&EthTxManager::ContinueSpeedupOrCancelTransaction,
                       weak_factory_.GetWeakPtr(), meta->chain_id(),
                       meta->from().Clone(), meta->origin(),
                       Uint256ValueToHex(meta->tx()->gas_limit()),
                       std::move(tx), std::move(callback)));
  }
}

void EthTxManager::ContinueSpeedupOrCancelTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    const std::string& gas_limit,
    std::unique_ptr<EthTransaction> tx,
    SpeedupOrCancelTransactionCallback callback,
    const std::string& result,
    mojom::ProviderError error,
    const std::string& error_message) {
  uint256_t latest_estimate_gas_price;
  if (error != mojom::ProviderError::kSuccess ||
      !HexValueToUint256(result, &latest_estimate_gas_price)) {
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

  ContinueAddUnapprovedTransaction(chain_id, from, origin, std::move(tx),
                                   std::move(callback), false, gas_limit,
                                   mojom::ProviderError::kSuccess, "");
}

void EthTxManager::ContinueSpeedupOrCancel1559Transaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
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

  ContinueAddUnapprovedTransaction(chain_id, from, origin, std::move(tx),
                                   std::move(callback), false, gas_limit,
                                   mojom::ProviderError::kSuccess, "");
}

void EthTxManager::RetryTransaction(const std::string& tx_meta_id,
                                    RetryTransactionCallback callback) {
  std::unique_ptr<EthTxMeta> meta = GetEthTxStateManager().GetEthTx(tx_meta_id);

  if (!meta || !meta->tx()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  if (!meta->IsRetriable()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_RETRIABLE));
    return;
  }

  std::unique_ptr<EthTransaction> tx;
  if (meta->tx()->type() == 2) {  // EIP1559
    tx = std::make_unique<Eip1559Transaction>(
        *static_cast<Eip1559Transaction*>(meta->tx()));
  } else {
    tx = std::make_unique<EthTransaction>(*meta->tx());
  }

  ContinueAddUnapprovedTransaction(
      meta->chain_id(), meta->from(), meta->origin(), std::move(tx),
      std::move(callback), false, Uint256ValueToHex(meta->tx()->gas_limit()),
      mojom::ProviderError::kSuccess, "");
}

void EthTxManager::GetGasEstimation1559(const std::string& chain_id,
                                        GetGasEstimation1559Callback callback) {
  json_rpc_service_->GetFeeHistory(
      chain_id, base::BindOnce(&EthTxManager::OnGetGasEstimation1559,
                               weak_factory_.GetWeakPtr(), std::move(callback),
                               chain_id));
}

void EthTxManager::OnGetGasEstimation1559(
    GetGasEstimation1559Callback callback,
    const std::string& chain_id,
    const std::vector<std::string>& base_fee_per_gas,
    const std::vector<double>& gas_used_ratio,
    const std::string& oldest_block,
    const std::vector<std::vector<std::string>>& reward,
    mojom::ProviderError error,
    const std::string& error_message) {
  // If eth_feeHistory method was not found, try to get the base fee
  // from eth_getBlockByNumber.
  if (error == mojom::ProviderError::kMethodNotFound) {
    json_rpc_service_->GetBaseFeePerGas(
        chain_id,
        base::BindOnce(&EthTxManager::OnGetBaseFeePerGas,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run(nullptr);
    return;
  }

  uint256_t low_priority_fee;
  uint256_t avg_priority_fee;
  uint256_t high_priority_fee;
  uint256_t suggested_base_fee_per_gas;
  if (!eth::GetSuggested1559Fees(base_fee_per_gas, gas_used_ratio, oldest_block,
                                 reward, &low_priority_fee, &avg_priority_fee,
                                 &high_priority_fee,
                                 &suggested_base_fee_per_gas)) {
    std::move(callback).Run(nullptr);
    return;
  }

  mojom::GasEstimation1559Ptr estimation = mojom::GasEstimation1559::New();
  estimation->base_fee_per_gas = Uint256ValueToHex(suggested_base_fee_per_gas);
  estimation->slow_max_priority_fee_per_gas =
      Uint256ValueToHex(low_priority_fee);
  estimation->avg_max_priority_fee_per_gas =
      Uint256ValueToHex(avg_priority_fee);
  estimation->fast_max_priority_fee_per_gas =
      Uint256ValueToHex(high_priority_fee);
  estimation->slow_max_fee_per_gas =
      Uint256ValueToHex(suggested_base_fee_per_gas + low_priority_fee);
  estimation->avg_max_fee_per_gas =
      Uint256ValueToHex(suggested_base_fee_per_gas + avg_priority_fee);
  estimation->fast_max_fee_per_gas =
      Uint256ValueToHex(suggested_base_fee_per_gas + high_priority_fee);
  std::move(callback).Run(std::move(estimation));
}

void EthTxManager::OnGetBaseFeePerGas(GetGasEstimation1559Callback callback,
                                      const std::string& base_fee_per_gas,
                                      mojom::ProviderError error,
                                      const std::string& error_message) {
  if (base_fee_per_gas.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }

  mojom::GasEstimation1559Ptr estimation = mojom::GasEstimation1559::New();
  auto scaled_base_fee_per_gas_uint256 =
      eth::ScaleBaseFeePerGas(base_fee_per_gas);
  if (!scaled_base_fee_per_gas_uint256) {
    std::move(callback).Run(nullptr);
    return;
  }

  const auto& scaled_base_fee_per_gas =
      Uint256ValueToHex(*scaled_base_fee_per_gas_uint256);

  estimation->base_fee_per_gas = scaled_base_fee_per_gas;
  estimation->slow_max_priority_fee_per_gas = "0x0";
  estimation->avg_max_priority_fee_per_gas = "0x0";
  estimation->fast_max_priority_fee_per_gas = "0x0";
  estimation->slow_max_fee_per_gas = scaled_base_fee_per_gas;
  estimation->avg_max_fee_per_gas = scaled_base_fee_per_gas;
  estimation->fast_max_fee_per_gas = scaled_base_fee_per_gas;
  std::move(callback).Run(std::move(estimation));
}

void EthTxManager::Reset() {
  TxManager::Reset();
  pending_tx_tracker_->Reset();
}

EthTxStateManager& EthTxManager::GetEthTxStateManager() {
  return static_cast<EthTxStateManager&>(tx_state_manager());
}

EthBlockTracker& EthTxManager::GetEthBlockTracker() {
  return static_cast<EthBlockTracker&>(block_tracker());
}

}  // namespace brave_wallet
