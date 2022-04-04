/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/eth_sign_typed_data_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

std::unique_ptr<base::Value> GetJsonRpcRequest(
    const std::string& method,
    std::unique_ptr<base::Value> params) {
  auto dictionary =
      base::Value::ToUniquePtrValue(base::Value(base::Value::Type::DICTIONARY));
  dictionary->SetKey("jsonrpc", base::Value("2.0"));
  dictionary->SetKey("method", base::Value(method));
  dictionary->SetKey("params", params->Clone());
  dictionary->SetKey("id", base::Value("1"));
  return dictionary;
}

}  // namespace

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    HostContentSettingsMap* host_content_settings_map,
    JsonRpcService* json_rpc_service,
    TxService* tx_service,
    KeyringService* keyring_service,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      json_rpc_service_(json_rpc_service),
      tx_service_(tx_service),
      keyring_service_(keyring_service),
      brave_wallet_service_(brave_wallet_service),
      prefs_(prefs),
      weak_factory_(this) {
  DCHECK(json_rpc_service);
  json_rpc_service_->AddObserver(
      rpc_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_service);
  tx_service_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(keyring_service);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
  host_content_settings_map_->AddObserver(this);

  // Get the current so we can compare for changed events
  if (delegate_)
    UpdateKnownAccounts();
}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {
  host_content_settings_map_->RemoveObserver(this);
}

void BraveWalletProviderImpl::AddEthereumChain(const std::string& json_payload,
                                               RequestCallback callback,
                                               base::Value id) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (json_payload.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }

  auto json_value = base::JSONReader::Read(
      json_payload,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }

  const base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }
  const auto& list = params->GetList();
  if (list.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_EXPECTED_SINGLE_PARAMETER));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }
  auto chain = brave_wallet::ValueToEthNetworkInfo(*list.begin());
  if (!chain) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }

  // Check if we already have the chain
  if (GetNetworkURL(prefs_, chain->chain_id, mojom::CoinType::ETH).is_valid()) {
    if (json_rpc_service_->GetChainId(mojom::CoinType::ETH) !=
        chain->chain_id) {
      SwitchEthereumChain(chain->chain_id, std::move(callback), std::move(id));
      return;
    }

    reject = false;
    std::move(callback).Run(std::move(id), base::Value(), reject, "", true);
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain->chain_id.empty() || chain->rpc_urls.empty() ||
      chain->chain_name.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }
  if (chain_callbacks_.contains(chain->chain_id)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }
  if (!delegate_) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", true);
    return;
  }
  chain_callbacks_[chain->chain_id] = std::move(callback);
  chain_ids_[chain->chain_id] = std::move(id);
  json_rpc_service_->AddEthereumChainForOrigin(
      chain->Clone(), delegate_->GetOrigin(),
      base::BindOnce(&BraveWalletProviderImpl::OnAddEthereumChain,
                     weak_factory_.GetWeakPtr()));
}

void BraveWalletProviderImpl::OnAddEthereumChain(
    const std::string& chain_id,
    mojom::ProviderError error,
    const std::string& error_message) {
  DCHECK(delegate_);
  if (!chain_callbacks_.contains(chain_id) || !chain_ids_.contains(chain_id))
    return;
  if (error != mojom::ProviderError::kSuccess) {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    bool reject = true;
    std::move(chain_callbacks_[chain_id])
        .Run(std::move(chain_ids_[chain_id]), std::move(*formed_response),
             reject, "", true);
    chain_callbacks_.erase(chain_id);
    chain_ids_.erase(chain_id);
    return;
  }
  delegate_->ShowPanel();
}

void BraveWalletProviderImpl::SwitchEthereumChain(const std::string& chain_id,
                                                  RequestCallback callback,
                                                  base::Value id) {
  // Only show bubble when there is no immediate error
  if (json_rpc_service_->AddSwitchEthereumChainRequest(
          chain_id, delegate_->GetOrigin(), std::move(callback), std::move(id)))
    delegate_->ShowPanel();
}

void BraveWalletProviderImpl::ContinueGetDefaultKeyringInfo(
    RequestCallback callback,
    base::Value id,
    const std::string& normalized_json_request,
    mojom::NetworkInfoPtr chain) {
  keyring_service_->GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindOnce(
          &BraveWalletProviderImpl::OnGetNetworkAndDefaultKeyringInfo,
          weak_factory_.GetWeakPtr(), std::move(callback), std::move(id),
          normalized_json_request, std::move(chain)));
}

void BraveWalletProviderImpl::OnGetNetworkAndDefaultKeyringInfo(
    RequestCallback callback,
    base::Value id,
    const std::string& normalized_json_request,
    mojom::NetworkInfoPtr chain,
    mojom::KeyringInfoPtr keyring_info) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (!chain || !keyring_info) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::string from;
  mojom::TxData1559Ptr tx_data_1559 =
      ParseEthSendTransaction1559Params(normalized_json_request, &from);
  if (!tx_data_1559) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  bool is_eip1559 = false;
  if (chain->data && chain->data->is_eth_data()) {
    is_eip1559 = chain->data->get_eth_data()->is_eip1559;
  }
  if (ShouldCreate1559Tx(tx_data_1559.Clone(), is_eip1559,
                         keyring_info->account_infos, from)) {
    // Set chain_id to current chain_id.
    tx_data_1559->chain_id = chain->chain_id;
    // If the chain id is not known yet, then get it and set it first
    if (tx_data_1559->chain_id == "0x0" || tx_data_1559->chain_id.empty()) {
      json_rpc_service_->GetChainId(
          mojom::CoinType::ETH,
          base::BindOnce(
              &BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction,
              weak_factory_.GetWeakPtr(), std::move(callback), std::move(id),
              std::move(tx_data_1559), from));
    } else {
      GetAllowedAccounts(
          false,
          base::BindOnce(&BraveWalletProviderImpl::
                             ContinueAddAndApprove1559TransactionWithAccounts,
                         weak_factory_.GetWeakPtr(), std::move(callback),
                         std::move(id), std::move(tx_data_1559), from));
    }
  } else {
    if (!tx_data_1559) {
      formed_response = GetProviderErrorDictionary(
          brave_wallet::mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
      reject = true;
      std::move(callback).Run(std::move(id), std::move(*formed_response),
                              reject, "", false);
      return;
    }

    GetAllowedAccounts(
        false, base::BindOnce(
                   &BraveWalletProviderImpl::ContinueAddAndApproveTransaction,
                   weak_factory_.GetWeakPtr(), std::move(callback),
                   std::move(id), std::move(tx_data_1559->base_data), from));
  }
}

void BraveWalletProviderImpl::IsLocked(IsLockedCallback callback) {
  keyring_service_->IsLocked(std::move(callback));
}

void BraveWalletProviderImpl::ContinueAddAndApproveTransaction(
    RequestCallback callback,
    base::Value id,
    mojom::TxDataPtr tx_data,
    const std::string& from,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData(std::move(tx_data)), from,
      base::BindOnce(
          &BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter,
          weak_factory_.GetWeakPtr(), std::move(callback), std::move(id)));
}

// AddUnapprovedTransaction is a different return type from
// AddAndApproveTransaction so we need to use an adapter callback that passses
// through.
void BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter(
    RequestCallback callback,
    base::Value id,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  OnAddUnapprovedTransaction(std::move(callback), std::move(id), tx_meta_id,
                             success ? mojom::ProviderError::kSuccess
                                     : mojom::ProviderError::kInternalError,
                             success ? "" : error_message);
}

void BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction(
    RequestCallback callback,
    base::Value id,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const std::string& chain_id) {
  tx_data->chain_id = chain_id;
  GetAllowedAccounts(
      false,
      base::BindOnce(&BraveWalletProviderImpl::
                         ContinueAddAndApprove1559TransactionWithAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), std::move(tx_data), from));
}

void BraveWalletProviderImpl::ContinueAddAndApprove1559TransactionWithAccounts(
    RequestCallback callback,
    base::Value id,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData1559(std::move(tx_data)), from,
      base::BindOnce(
          &BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter,
          weak_factory_.GetWeakPtr(), std::move(callback), std::move(id)));
}

void BraveWalletProviderImpl::OnAddUnapprovedTransaction(
    RequestCallback callback,
    base::Value id,
    const std::string& tx_meta_id,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error == mojom::ProviderError::kSuccess) {
    add_tx_callbacks_[tx_meta_id] = std::move(callback);
    add_tx_ids_[tx_meta_id] = std::move(id);
    delegate_->ShowPanel();
  } else {
    std::unique_ptr<base::Value> formed_response =
        GetProviderErrorDictionary(error, error_message);
    bool reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
  }
}

void BraveWalletProviderImpl::SignMessage(const std::string& address,
                                          const std::string& message,
                                          RequestCallback callback,
                                          base::Value id) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (!EthAddress::IsValidAddress(address) || !IsValidHexString(message)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::string message_str(message_bytes.begin(), message_bytes.end());
  if (!base::IsStringUTF8(message_str))
    message_str = ToHex(message_str);

  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(
      false,
      base::BindOnce(&BraveWalletProviderImpl::ContinueSignMessage,
                     weak_factory_.GetWeakPtr(),
                     checksum_address.ToChecksumAddress(), message_str,
                     std::move(message_bytes), absl::nullopt, absl::nullopt,
                     false, std::move(callback), std::move(id)));
}

void BraveWalletProviderImpl::RecoverAddress(const std::string& message,
                                             const std::string& signature,
                                             RequestCallback callback,
                                             base::Value id) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  // 65 * 2 hex chars per byte + 2 chars for  0x
  if (signature.length() != 132) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> signature_bytes;
  if (!PrefixedHexStringToBytes(signature, &signature_bytes)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  std::string address;
  if (!keyring_service_->RecoverAddressByDefaultKeyring(
          message_bytes, signature_bytes, &address)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  formed_response = base::Value::ToUniquePtrValue(base::Value(address));
  reject = false;
  std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                          "", false);
}

void BraveWalletProviderImpl::SignTypedMessage(
    const std::string& address,
    const std::string& message,
    const std::vector<uint8_t>& domain_hash,
    const std::vector<uint8_t>& primary_hash,
    base::Value domain,
    RequestCallback callback,
    base::Value id) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (!EthAddress::IsValidAddress(address) || !domain.is_dict() ||
      domain_hash.empty() || primary_hash.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }
  auto chain_id = domain.FindDoubleKey("chainId");
  if (chain_id) {
    const std::string chain_id_hex =
        Uint256ValueToHex((uint256_t)(uint64_t)*chain_id);
    if (chain_id_hex != json_rpc_service_->GetChainId(mojom::CoinType::ETH)) {
      formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_TYPED_MESSAGE_CHAIN_ID_MISMATCH,
              base::ASCIIToUTF16(chain_id_hex)));
      reject = true;
      std::move(callback).Run(std::move(id), std::move(*formed_response),
                              reject, "", false);
      return;
    }
  }

  if (domain_hash.empty() || primary_hash.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }
  auto message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      domain_hash, primary_hash);
  if (!message_to_sign || message_to_sign->size() != 32) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(
      false,
      base::BindOnce(&BraveWalletProviderImpl::ContinueSignMessage,
                     weak_factory_.GetWeakPtr(),
                     checksum_address.ToChecksumAddress(), message,
                     std::move(*message_to_sign), base::HexEncode(domain_hash),
                     base::HexEncode(primary_hash), true, std::move(callback),
                     std::move(id)));
}

void BraveWalletProviderImpl::ContinueSignMessage(
    const std::string& address,
    const std::string& message,
    std::vector<uint8_t>&& message_to_sign,
    const absl::optional<std::string>& domain_hash,
    const absl::optional<std::string>& primary_hash,
    bool is_eip712,
    RequestCallback callback,
    base::Value id,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(address, allowed_accounts)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                  base::ASCIIToUTF16(address)));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  auto request =
      mojom::SignMessageRequest::New(sign_message_id_++, address, message,
                                     is_eip712, domain_hash, primary_hash);

  if (keyring_service_->IsHardwareAccount(address)) {
    brave_wallet_service_->AddSignMessageRequest(
        std::move(request),
        base::BindOnce(
            &BraveWalletProviderImpl::OnHardwareSignMessageRequestProcessed,
            weak_factory_.GetWeakPtr(), std::move(callback), std::move(id),
            address, std::move(message_to_sign), is_eip712));
  } else {
    brave_wallet_service_->AddSignMessageRequest(
        std::move(request),
        base::BindOnce(&BraveWalletProviderImpl::OnSignMessageRequestProcessed,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), address, std::move(message_to_sign),
                       is_eip712));
  }
  delegate_->ShowPanel();
}

void BraveWalletProviderImpl::OnSignMessageRequestProcessed(
    RequestCallback callback,
    base::Value id,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool is_eip712,
    bool approved,
    const std::string& signature,
    const std::string& error) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (!approved) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  auto signature_with_err = keyring_service_->SignMessageByDefaultKeyring(
      address, message, is_eip712);
  if (!signature_with_err.signature) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError, signature_with_err.error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
  } else {
    formed_response = base::Value::ToUniquePtrValue(
        base::Value(ToHex(*signature_with_err.signature)));
    reject = false;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
  }
}

void BraveWalletProviderImpl::OnHardwareSignMessageRequestProcessed(
    RequestCallback callback,
    base::Value id,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool is_eip712,
    bool approved,
    const std::string& signature,
    const std::string& error) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = false;
  if (!approved) {
    mojom::ProviderError error_code =
        error.empty() ? mojom::ProviderError::kUserRejectedRequest
                      : mojom::ProviderError::kInternalError;
    auto error_message =
        error.empty()
            ? l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST)
            : error;
    formed_response = GetProviderErrorDictionary(error_code, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  formed_response = base::Value::ToUniquePtrValue(base::Value(signature));
  reject = false;
  std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                          "", false);
}

bool BraveWalletProviderImpl::CheckAccountAllowed(
    const std::string& account,
    const std::vector<std::string>& allowed_accounts) {
  for (const auto& allowed_account : allowed_accounts) {
    if (base::EqualsCaseInsensitiveASCII(account, allowed_account)) {
      return true;
    }
  }
  return false;
}

void BraveWalletProviderImpl::OnAddEthereumChainRequestCompleted(
    const std::string& chain_id,
    const std::string& error) {
  if (!chain_callbacks_.contains(chain_id) || !chain_ids_.contains(chain_id))
    return;
  if (error.empty()) {
    // To match MM for webcompat, after adding a chain we should prompt
    // again to switch to the chain. And the error result only depends on
    // what the switch action is at that point.
    SwitchEthereumChain(chain_id, std::move(chain_callbacks_[chain_id]),
                        std::move(chain_ids_[chain_id]));
    chain_callbacks_.erase(chain_id);
    chain_ids_.erase(chain_id);
    return;
  }
  bool reject = true;
  std::unique_ptr<base::Value> formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest, error);
  std::move(chain_callbacks_[chain_id])
      .Run(std::move(chain_ids_[chain_id]), std::move(*formed_response), reject,
           "", false);

  chain_callbacks_.erase(chain_id);
  chain_ids_.erase(chain_id);
}

void BraveWalletProviderImpl::Request(base::Value input,
                                      const std::string& origin,
                                      RequestCallback callback) {
  CommonRequestOrSendAsync(std::move(input), origin, std::move(callback));
}

void BraveWalletProviderImpl::SendErrorOnRequest(
    const mojom::ProviderError& error,
    const std::string& error_message,
    RequestCallback callback,
    base::Value id) {
  std::unique_ptr<base::Value> formed_response =
      GetProviderErrorDictionary(error, error_message);
  std::move(callback).Run(std::move(id), std::move(*formed_response), true, "",
                          false);
}

void BraveWalletProviderImpl::CommonRequestOrSendAsync(
    base::Value input_value,
    const std::string& origin,
    RequestCallback callback) {
  mojom::ProviderError error = mojom::ProviderError::kUnsupportedMethod;
  std::string error_message = "Generic processing error";
  DCHECK(json_rpc_service_);
  std::string input_json;
  if (!base::JSONWriter::Write(input_value, &input_json) ||
      input_json.empty()) {
    SendErrorOnRequest(error, error_message, std::move(callback),
                       base::Value());
    return;
  }

  std::string normalized_json_request;
  if (!NormalizeEthRequest(input_json, &normalized_json_request)) {
    SendErrorOnRequest(error, error_message, std::move(callback),
                       base::Value());
    return;
  }

  base::Value id;
  std::string method;
  if (!GetEthJsonRequestInfo(normalized_json_request, &id, &method, nullptr)) {
    SendErrorOnRequest(error, error_message, std::move(callback),
                       base::Value());
    return;
  }

  if (method == kEthAccounts) {
    GetAllowedAccounts(
        false,
        base::BindOnce(&BraveWalletProviderImpl::OnContinueGetAllowedAccounts,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, origin));
  } else if (method == kEthRequestAccounts) {
    RequestEthereumPermissions(std::move(callback), std::move(id), method,
                               origin);
  } else if (method == kAddEthereumChainMethod) {
    AddEthereumChain(normalized_json_request, std::move(callback),
                     std::move(id));
  } else if (method == kSwitchEthereumChainMethod) {
    std::string chain_id;
    if (!ParseSwitchEthereumChainParams(normalized_json_request, &chain_id)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    SwitchEthereumChain(chain_id, std::move(callback), std::move(id));
  } else if (method == kEthSendTransaction) {
    json_rpc_service_->GetNetwork(
        mojom::CoinType::ETH,
        base::BindOnce(&BraveWalletProviderImpl::ContinueGetDefaultKeyringInfo,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), normalized_json_request));
  } else if (method == kEthSign || method == kPersonalSign) {
    std::string address;
    std::string message;
    if (method == kPersonalSign &&
        !ParsePersonalSignParams(normalized_json_request, &address, &message)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    } else if (method == kEthSign &&
               !ParseEthSignParams(normalized_json_request, &address,
                                   &message)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    SignMessage(address, message, std::move(callback), std::move(id));
  } else if (method == kPersonalEcRecover) {
    std::string message;
    std::string signature;
    if (!ParsePersonalEcRecoverParams(normalized_json_request, &message,
                                      &signature)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    RecoverAddress(message, signature, std::move(callback), std::move(id));
  } else if (method == kEthSignTypedDataV3 || method == kEthSignTypedDataV4) {
    std::string address;
    std::string message;
    base::Value domain;
    std::vector<uint8_t> message_to_sign;
    std::vector<uint8_t> domain_hash_out;
    std::vector<uint8_t> primary_hash_out;
    if (method == kEthSignTypedDataV4) {
      if (!ParseEthSignTypedDataParams(normalized_json_request, &address,
                                       &message, &domain,
                                       EthSignTypedDataHelper::Version::kV4,
                                       &domain_hash_out, &primary_hash_out)) {
        SendErrorOnRequest(error, error_message, std::move(callback),
                           std::move(id));
        return;
      }
    } else {
      if (!ParseEthSignTypedDataParams(normalized_json_request, &address,
                                       &message, &domain,
                                       EthSignTypedDataHelper::Version::kV3,
                                       &domain_hash_out, &primary_hash_out)) {
        SendErrorOnRequest(error, error_message, std::move(callback),
                           std::move(id));
        return;
      }
    }

    SignTypedMessage(address, message, domain_hash_out, primary_hash_out,
                     std::move(domain), std::move(callback), std::move(id));
  } else if (method == kWalletWatchAsset || method == kMetamaskWatchAsset) {
    mojom::BlockchainTokenPtr token;
    if (!ParseWalletWatchAssetParams(normalized_json_request, &token,
                                     &error_message)) {
      if (!error_message.empty())
        error = mojom::ProviderError::kInvalidParams;
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    AddSuggestToken(std::move(token), std::move(callback), std::move(id));
  } else if (method == kRequestPermissionsMethod) {
    std::vector<std::string> restricted_methods;
    if (!ParseRequestPermissionsParams(normalized_json_request,
                                       &restricted_methods)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    if (std::find(restricted_methods.begin(), restricted_methods.end(),
                  "eth_accounts") == restricted_methods.end()) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }

    RequestEthereumPermissions(std::move(callback), std::move(id), method,
                               origin);
  } else if (method == kGetPermissionsMethod) {
    GetAllowedAccounts(
        true,
        base::BindOnce(&BraveWalletProviderImpl::OnContinueGetAllowedAccounts,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, origin));
  } else {
    json_rpc_service_->Request(normalized_json_request, true, std::move(id),
                               mojom::CoinType::ETH, std::move(callback));
  }
}

void BraveWalletProviderImpl::Send(const std::string& method,
                                   base::Value params,
                                   const std::string& origin,
                                   SendCallback callback) {
  std::unique_ptr<base::Value> params_ptr =
      base::Value::ToUniquePtrValue(std::move(params));
  CommonRequestOrSendAsync(
      std::move(*GetJsonRpcRequest(method, std::move(params_ptr))), origin,
      std::move(callback));
}

void BraveWalletProviderImpl::RequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const std::string& origin) {
  DCHECK(delegate_);
  delegate_->RequestEthereumPermissions(
      base::BindOnce(&BraveWalletProviderImpl::OnRequestEthereumPermissions,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), method, origin));
}

void BraveWalletProviderImpl::Enable(EnableCallback callback) {
  RequestEthereumPermissions(std::move(callback), base::Value(), "", "");
}

void BraveWalletProviderImpl::OnRequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const std::string& origin,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::unique_ptr<base::Value> formed_response;
  bool reject = error != mojom::ProviderError::kSuccess;
  if (error == mojom::ProviderError::kSuccess && keyring_service_->IsLocked()) {
    if (pending_request_ethereum_permissions_callback_) {
      formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kUserRejectedRequest,
          l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
      reject = true;
      std::move(callback).Run(std::move(id), std::move(*formed_response),
                              reject, "", true);
      return;
    }
    pending_request_ethereum_permissions_callback_ = std::move(callback);
    pending_request_ethereum_permissions_id_ = std::move(id);
    pending_request_ethereum_permissions_method_ = method;
    pending_request_ethereum_permissions_origin_ = origin;
    keyring_service_->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  bool success = error == mojom::ProviderError::kSuccess;
  std::string first_allowed_account;
  if (accounts.size() > 0) {
    first_allowed_account = accounts[0];
  }
  if (success && accounts.empty()) {
    formed_response =
        GetProviderErrorDictionary(mojom::ProviderError::kUserRejectedRequest,
                                   "User rejected the request.");
  } else if (!success) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else if (method == kRequestPermissionsMethod) {
    formed_response = base::Value::ToUniquePtrValue(
        PermissionRequestResponseToValue(origin, accounts));
  } else {
    formed_response = base::Value::ToUniquePtrValue(base::ListValue());
    for (size_t i = 0; i < accounts.size(); i++) {
      formed_response->Append(base::Value(accounts[i]));
    }
  }
  reject = !success || accounts.empty();

  std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                          first_allowed_account, true);
}

void BraveWalletProviderImpl::GetAllowedAccounts(
    bool include_accounts_when_locked,
    GetAllowedAccountsCallback callback) {
  DCHECK(delegate_);
  delegate_->GetAllowedAccounts(
      include_accounts_when_locked,
      base::BindOnce(&BraveWalletProviderImpl::OnGetAllowedAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnGetAllowedAccounts(
    GetAllowedAccountsCallback callback,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  std::move(callback).Run(accounts, error, error_message);
}

void BraveWalletProviderImpl::OnContinueGetAllowedAccounts(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const std::string& origin,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  bool reject = error != mojom::ProviderError::kSuccess;
  bool update_bindings = false;
  std::unique_ptr<base::Value> formed_response;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else if (method == kEthAccounts) {
    formed_response = base::Value::ToUniquePtrValue(base::ListValue());
    for (size_t i = 0; i < accounts.size(); i++) {
      formed_response->Append(base::Value(accounts[i]));
    }
    update_bindings = false;
  } else {
    formed_response = base::Value::ToUniquePtrValue(
        PermissionRequestResponseToValue(origin, accounts));
    update_bindings = true;
  }
  std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                          "", update_bindings);
}

void BraveWalletProviderImpl::UpdateKnownAccounts() {
  GetAllowedAccounts(
      false, base::BindOnce(&BraveWalletProviderImpl::OnUpdateKnownAccounts,
                            weak_factory_.GetWeakPtr()));
}

void BraveWalletProviderImpl::OnUpdateKnownAccounts(
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    return;
  }
  bool accounts_changed = allowed_accounts != known_allowed_accounts;
  known_allowed_accounts = allowed_accounts;
  if (!first_known_accounts_check && events_listener_.is_bound() &&
      accounts_changed) {
    events_listener_->AccountsChangedEvent(known_allowed_accounts);
  }
  first_known_accounts_check = false;
}

void BraveWalletProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (json_rpc_service_) {
    json_rpc_service_->GetChainId(mojom::CoinType::ETH, std::move(callback));
  }
}

void BraveWalletProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void BraveWalletProviderImpl::ChainChangedEvent(const std::string& chain_id,
                                                mojom::CoinType coin) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::ETH)
    return;

  events_listener_->ChainChangedEvent(chain_id);
}

void BraveWalletProviderImpl::OnConnectionError() {
  rpc_observer_receiver_.reset();
  tx_observer_receiver_.reset();
  keyring_observer_receiver_.reset();
}

void BraveWalletProviderImpl::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  auto tx_status = tx_info->tx_status;
  if (tx_status != mojom::TransactionStatus::Submitted &&
      tx_status != mojom::TransactionStatus::Rejected &&
      tx_status != mojom::TransactionStatus::Error)
    return;

  std::string tx_meta_id = tx_info->id;
  if (!add_tx_callbacks_.contains(tx_meta_id) ||
      !add_tx_ids_.contains(tx_meta_id))
    return;

  std::string tx_hash = tx_info->tx_hash;
  std::unique_ptr<base::Value> formed_response;
  bool reject = true;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    formed_response = base::Value::ToUniquePtrValue(base::Value(tx_hash));
    reject = false;
  } else if (tx_status == mojom::TransactionStatus::Rejected) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
    reject = true;
  } else if (tx_status == mojom::TransactionStatus::Error) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_ERROR));
    reject = true;
  }
  std::move(add_tx_callbacks_[tx_meta_id])
      .Run(std::move(add_tx_ids_[tx_meta_id]), std::move(*formed_response),
           reject, "", false);
  add_tx_callbacks_.erase(tx_meta_id);
  add_tx_ids_.erase(tx_meta_id);
}

void BraveWalletProviderImpl::SelectedAccountChanged(mojom::CoinType coin) {
  if (coin != mojom::CoinType::ETH)
    return;
  UpdateKnownAccounts();
}

void BraveWalletProviderImpl::Locked() {
  UpdateKnownAccounts();
}

void BraveWalletProviderImpl::Unlocked() {
  if (pending_request_ethereum_permissions_callback_) {
    RequestEthereumPermissions(
        std::move(pending_request_ethereum_permissions_callback_),
        std::move(pending_request_ethereum_permissions_id_),
        pending_request_ethereum_permissions_method_,
        pending_request_ethereum_permissions_origin_);
  } else {
    UpdateKnownAccounts();
  }
}

void BraveWalletProviderImpl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::BRAVE_ETHEREUM) {
    UpdateKnownAccounts();
  }
}

void BraveWalletProviderImpl::AddSuggestToken(mojom::BlockchainTokenPtr token,
                                              RequestCallback callback,
                                              base::Value id) {
  if (!token) {
    std::unique_ptr<base::Value> formed_response;
    bool reject = true;
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  auto request = mojom::AddSuggestTokenRequest::New(std::move(token));
  brave_wallet_service_->AddSuggestTokenRequest(
      std::move(request), std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

}  // namespace brave_wallet
