/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_provider_impl.h"

#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_macros.h"
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
#include "url/origin.h"

namespace {

base::Value::Dict GetJsonRpcRequest(const std::string& method,
                                    base::Value params) {
  base::Value::Dict dictionary;
  dictionary.Set("jsonrpc", "2.0");
  dictionary.Set("method", method);
  dictionary.Set("params", std::move(params));
  dictionary.Set("id", "1");
  return dictionary;
}

// Common logic for filtering the list of accounts based on the selected account
std::vector<std::string> FilterAccounts(
    const std::vector<std::string>& accounts,
    const absl::optional<std::string>& selected_account) {
  // If one of the accounts matches the selected account, then only
  // return that account.  This is for webcompat reasons.
  // Some Dapps select the first account in the list, and some the
  // last. So having only 1 item returned here makes it work for
  // all Dapps.
  std::vector<std::string> filtered_accounts;
  for (const auto& account : accounts) {
    if (selected_account &&
        base::CompareCaseInsensitiveASCII(account, *selected_account) == 0) {
      filtered_accounts.clear();
      filtered_accounts.push_back(account);
      break;
    } else {
      filtered_accounts.push_back(account);
    }
  }
  return filtered_accounts;
}

}  // namespace

namespace brave_wallet {

EthereumProviderImpl::EthereumProviderImpl(
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

EthereumProviderImpl::~EthereumProviderImpl() {
  host_content_settings_map_->RemoveObserver(this);
}

void EthereumProviderImpl::AddEthereumChain(const std::string& json_payload,
                                            RequestCallback callback,
                                            base::Value id) {
  bool reject = false;
  if (json_payload.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }

  auto json_value = base::JSONReader::Read(
      json_payload,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }

  const base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  const auto& list = params->GetList();
  if (list.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_EXPECTED_SINGLE_PARAMETER));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  auto chain = ParseEip3085Payload(*list.begin());
  if (!chain) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  std::string chain_id_lower = base::ToLowerASCII(chain->chain_id);

  // Check if we already have the chain
  if (GetNetworkURL(prefs_, chain_id_lower, mojom::CoinType::ETH).is_valid()) {
    if (base::CompareCaseInsensitiveASCII(
            json_rpc_service_->GetChainId(mojom::CoinType::ETH),
            chain_id_lower) != 0) {
      SwitchEthereumChain(chain_id_lower, std::move(callback), std::move(id));
      return;
    }

    reject = false;
    std::move(callback).Run(std::move(id), base::Value(), reject, "", true);
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain_id_lower.empty() || chain->rpc_endpoints.empty() ||
      chain->chain_name.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  if (chain_callbacks_.contains(chain_id_lower)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  if (!delegate_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", true);
    return;
  }
  chain_callbacks_[chain_id_lower] = std::move(callback);
  chain_ids_[chain_id_lower] = std::move(id);
  json_rpc_service_->AddEthereumChainForOrigin(
      chain->Clone(), delegate_->GetOrigin(),
      base::BindOnce(&EthereumProviderImpl::OnAddEthereumChain,
                     weak_factory_.GetWeakPtr()));
}

void EthereumProviderImpl::OnAddEthereumChain(
    const std::string& chain_id,
    mojom::ProviderError error,
    const std::string& error_message) {
  DCHECK(delegate_);
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (!chain_callbacks_.contains(chain_id_lower) ||
      !chain_ids_.contains(chain_id_lower))
    return;
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    bool reject = true;
    std::move(chain_callbacks_[chain_id_lower])
        .Run(std::move(chain_ids_[chain_id_lower]), std::move(formed_response),
             reject, "", true);
    chain_callbacks_.erase(chain_id_lower);
    chain_ids_.erase(chain_id_lower);
    return;
  }
  delegate_->ShowPanel();
}

void EthereumProviderImpl::SwitchEthereumChain(const std::string& chain_id,
                                               RequestCallback callback,
                                               base::Value id) {
  // Only show bubble when there is no immediate error
  if (json_rpc_service_->AddSwitchEthereumChainRequest(
          chain_id, delegate_->GetOrigin(), std::move(callback), std::move(id)))
    delegate_->ShowPanel();
}

void EthereumProviderImpl::ContinueGetDefaultKeyringInfo(
    RequestCallback callback,
    base::Value id,
    const std::string& normalized_json_request,
    const url::Origin& origin,
    bool sign_only,
    mojom::NetworkInfoPtr chain) {
  keyring_service_->GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindOnce(&EthereumProviderImpl::OnGetNetworkAndDefaultKeyringInfo,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), normalized_json_request, origin,
                     std::move(chain), sign_only));
}

void EthereumProviderImpl::OnGetNetworkAndDefaultKeyringInfo(
    RequestCallback callback,
    base::Value id,
    const std::string& normalized_json_request,
    const url::Origin& origin,
    mojom::NetworkInfoPtr chain,
    bool sign_only,
    mojom::KeyringInfoPtr keyring_info) {
  bool reject = false;
  if (!chain || !keyring_info) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    base::Value formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::string from;
  mojom::TxData1559Ptr tx_data_1559 =
      ParseEthTransaction1559Params(normalized_json_request, &from);
  if (!tx_data_1559) {
    mojom::ProviderError code = mojom::ProviderError::kInternalError;
    std::string message = "Internal JSON-RPC error";
    base::Value formed_response = GetProviderErrorDictionary(code, message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  tx_data_1559->base_data->sign_only = sign_only;

  if (ShouldCreate1559Tx(tx_data_1559.Clone(), chain->is_eip1559,
                         keyring_info->account_infos, from)) {
    // Set chain_id to current chain_id.
    tx_data_1559->chain_id = chain->chain_id;
    // If the chain id is not known yet, then get it and set it first
    if (tx_data_1559->chain_id == "0x0" || tx_data_1559->chain_id.empty()) {
      json_rpc_service_->GetChainId(
          mojom::CoinType::ETH,
          base::BindOnce(
              &EthereumProviderImpl::ContinueAddAndApprove1559Transaction,
              weak_factory_.GetWeakPtr(), std::move(callback), std::move(id),
              std::move(tx_data_1559), from, origin));
    } else {
      GetAllowedAccounts(
          false,
          base::BindOnce(&EthereumProviderImpl::
                             ContinueAddAndApprove1559TransactionWithAccounts,
                         weak_factory_.GetWeakPtr(), std::move(callback),
                         std::move(id), std::move(tx_data_1559), from, origin));
    }
  } else {
    if (!tx_data_1559) {
      base::Value formed_response = GetProviderErrorDictionary(
          brave_wallet::mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
      reject = true;
      std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                              "", false);
      return;
    }

    GetAllowedAccounts(
        false,
        base::BindOnce(&EthereumProviderImpl::ContinueAddAndApproveTransaction,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), std::move(tx_data_1559->base_data), from,
                       origin));
  }
}

void EthereumProviderImpl::IsLocked(IsLockedCallback callback) {
  keyring_service_->IsLocked(std::move(callback));
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::ContinueAddAndApproveTransaction(
    RequestCallback callback,
    base::Value id,
    mojom::TxDataPtr tx_data,
    const std::string& from,
    const url::Origin& origin,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData(std::move(tx_data)), from, origin,
      absl::nullopt,
      base::BindOnce(&EthereumProviderImpl::OnAddUnapprovedTransactionAdapter,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id)));
}

// AddUnapprovedTransaction is a different return type from
// AddAndApproveTransaction so we need to use an adapter callback that passses
// through.
void EthereumProviderImpl::OnAddUnapprovedTransactionAdapter(
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

void EthereumProviderImpl::ContinueAddAndApprove1559Transaction(
    RequestCallback callback,
    base::Value id,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const url::Origin& origin,
    const std::string& chain_id) {
  tx_data->chain_id = chain_id;
  GetAllowedAccounts(
      false,
      base::BindOnce(&EthereumProviderImpl::
                         ContinueAddAndApprove1559TransactionWithAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), std::move(tx_data), from, origin));
}

void EthereumProviderImpl::ContinueAddAndApprove1559TransactionWithAccounts(
    RequestCallback callback,
    base::Value id,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const url::Origin& origin,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  tx_service_->AddUnapprovedTransaction(
      mojom::TxDataUnion::NewEthTxData1559(std::move(tx_data)), from, origin,
      absl::nullopt,
      base::BindOnce(&EthereumProviderImpl::OnAddUnapprovedTransactionAdapter,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id)));
}

void EthereumProviderImpl::OnAddUnapprovedTransaction(
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
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    bool reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
  }
}

void EthereumProviderImpl::SignMessage(const std::string& address,
                                       const std::string& message,
                                       RequestCallback callback,
                                       base::Value id) {
  bool reject = false;
  if (!EthAddress::IsValidAddress(address) || !IsValidHexString(message)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::string message_str(message_bytes.begin(), message_bytes.end());
  if (!base::IsStringUTF8(message_str))
    message_str = ToHex(message_str);

  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(
      false, base::BindOnce(&EthereumProviderImpl::ContinueSignMessage,
                            weak_factory_.GetWeakPtr(),
                            checksum_address.ToChecksumAddress(), message_str,
                            std::move(message_bytes), absl::nullopt,
                            absl::nullopt, false, std::move(callback),
                            std::move(id), delegate_->GetOrigin()));
}

void EthereumProviderImpl::RecoverAddress(const std::string& message,
                                          const std::string& signature,
                                          RequestCallback callback,
                                          base::Value id) {
  bool reject = false;
  // 65 * 2 hex chars per byte + 2 chars for  0x
  if (signature.length() != 132) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::vector<uint8_t> signature_bytes;
  if (!PrefixedHexStringToBytes(signature, &signature_bytes)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  std::string address;
  if (!keyring_service_->RecoverAddressByDefaultKeyring(
          message_bytes, signature_bytes, &address)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  reject = false;
  std::move(callback).Run(std::move(id), base::Value(address), reject, "",
                          false);
}

void EthereumProviderImpl::GetEncryptionPublicKey(const std::string& address,
                                                  RequestCallback callback,
                                                  base::Value id) {
  GetAllowedAccounts(
      false,
      base::BindOnce(&EthereumProviderImpl::ContinueGetEncryptionPublicKey,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), address, delegate_->GetOrigin()));
}

void EthereumProviderImpl::Decrypt(
    const std::string& untrusted_encrypted_data_json,
    const std::string& address,
    const url::Origin& origin,
    RequestCallback callback,
    base::Value id) {
  data_decoder::JsonSanitizer::Sanitize(
      untrusted_encrypted_data_json,
      base::BindOnce(&EthereumProviderImpl::ContinueDecryptWithSanitizedJson,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), address, origin));
}

void EthereumProviderImpl::ContinueDecryptWithSanitizedJson(
    RequestCallback callback,
    base::Value id,
    const std::string& address,
    const url::Origin& origin,
    data_decoder::JsonSanitizer::Result result) {
  if (result.error || !result.value.has_value()) {
    SendErrorOnRequest(mojom::ProviderError::kInvalidParams,
                       l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                       std::move(callback), std::move(id));
    return;
  }
  std::string validated_encrypted_data_json = result.value.value();
  std::string version;
  std::vector<uint8_t> nonce;
  std::vector<uint8_t> ephemeral_public_key;
  std::vector<uint8_t> ciphertext;
  if (!ParseEthDecryptData(validated_encrypted_data_json, &version, &nonce,
                           &ephemeral_public_key, &ciphertext)) {
    SendErrorOnRequest(mojom::ProviderError::kInvalidParams,
                       l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS),
                       std::move(callback), std::move(id));
    return;
  }

  GetAllowedAccounts(
      false,
      base::BindOnce(&EthereumProviderImpl::ContinueDecryptWithAllowedAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), version, nonce, ephemeral_public_key,
                     ciphertext, address, origin));
}

void EthereumProviderImpl::ContinueGetEncryptionPublicKey(
    RequestCallback callback,
    base::Value id,
    const std::string& address,
    const url::Origin& origin,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(address, allowed_accounts)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  // Only show bubble when there is no immediate error
  brave_wallet_service_->AddGetPublicKeyRequest(
      address, origin, std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::ContinueDecryptWithAllowedAccounts(
    RequestCallback callback,
    base::Value id,
    const std::string& version,
    const std::vector<uint8_t>& nonce,
    const std::vector<uint8_t>& ephemeral_public_key,
    const std::vector<uint8_t>& ciphertext,
    const std::string& address,
    const url::Origin& origin,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(address, allowed_accounts)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  absl::optional<std::vector<uint8_t>> unsafe_message_bytes =
      keyring_service_
          ->DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
              version, nonce, ephemeral_public_key, ciphertext, address);
  if (!unsafe_message_bytes.has_value()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  std::string unsafe_message(unsafe_message_bytes->begin(),
                             unsafe_message_bytes->end());
  // If the string was not UTF8 then it should have already failed on the
  // JSON sanitization, but we add this check for extra safety.
  if (!base::IsStringUTF8(unsafe_message)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(formed_response), false,
                            "", false);
    return;
  }

  auto request = mojom::DecryptRequest::New(MakeOriginInfo(origin), address,
                                            unsafe_message);
  brave_wallet_service_->AddDecryptRequest(std::move(request),
                                           std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::SignTypedMessage(
    const std::string& address,
    const std::string& message,
    const std::vector<uint8_t>& domain_hash,
    const std::vector<uint8_t>& primary_hash,
    base::Value::Dict domain,
    RequestCallback callback,
    base::Value id) {
  bool reject = false;
  if (!EthAddress::IsValidAddress(address) || domain_hash.empty() ||
      primary_hash.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  auto chain_id = domain.FindDouble("chainId");
  if (chain_id) {
    const std::string chain_id_hex =
        Uint256ValueToHex((uint256_t)(uint64_t)*chain_id);
    if (base::CompareCaseInsensitiveASCII(
            chain_id_hex,
            json_rpc_service_->GetChainId(mojom::CoinType::ETH)) != 0) {
      base::Value formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_TYPED_MESSAGE_CHAIN_ID_MISMATCH,
              base::ASCIIToUTF16(chain_id_hex)));
      reject = true;
      std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                              "", false);
      return;
    }
  }

  if (domain_hash.empty() || primary_hash.empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  auto message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      domain_hash, primary_hash);
  if (!message_to_sign || message_to_sign->size() != 32) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(
      false,
      base::BindOnce(&EthereumProviderImpl::ContinueSignMessage,
                     weak_factory_.GetWeakPtr(),
                     checksum_address.ToChecksumAddress(), message,
                     std::move(*message_to_sign), base::HexEncode(domain_hash),
                     base::HexEncode(primary_hash), true, std::move(callback),
                     std::move(id), delegate_->GetOrigin()));
}

void EthereumProviderImpl::ContinueSignMessage(
    const std::string& address,
    const std::string& message,
    std::vector<uint8_t>&& message_to_sign,
    const absl::optional<std::string>& domain_hash,
    const absl::optional<std::string>& primary_hash,
    bool is_eip712,
    RequestCallback callback,
    base::Value id,
    const url::Origin& origin,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  bool reject = false;
  if (error != mojom::ProviderError::kSuccess) {
    base::Value formed_response =
        GetProviderErrorDictionary(error, error_message);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  if (!CheckAccountAllowed(address, allowed_accounts)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                  base::ASCIIToUTF16(address)));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(origin), -1, address, message, is_eip712, domain_hash,
      primary_hash, absl::nullopt, mojom::CoinType::ETH);

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&EthereumProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), address, std::move(message_to_sign),
                     is_eip712));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::OnSignMessageRequestProcessed(
    RequestCallback callback,
    base::Value id,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool is_eip712,
    bool approved,
    mojom::ByteArrayStringUnionPtr signature,
    const absl::optional<std::string>& error) {
  bool reject = false;
  if (error && !error->empty()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInternalError, *error);
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  if (!approved) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  base::Value formed_response;
  if (!keyring_service_->IsHardwareAccount(address)) {
    auto signature_with_err = keyring_service_->SignMessageByDefaultKeyring(
        address, message, is_eip712);
    if (!signature_with_err.signature) {
      formed_response =
          GetProviderErrorDictionary(mojom::ProviderError::kInternalError,
                                     signature_with_err.error_message);
      reject = true;
    } else {
      formed_response = base::Value(ToHex(*signature_with_err.signature));
    }
  } else {
    if (!signature || !signature->is_str()) {  // Missing hardware signature.
      formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      reject = true;
    } else {
      formed_response = base::Value(signature->get_str());
    }
  }

  std::move(callback).Run(std::move(id), std::move(formed_response), reject, "",
                          false);
}

bool EthereumProviderImpl::CheckAccountAllowed(
    const std::string& account,
    const std::vector<std::string>& allowed_accounts) {
  for (const auto& allowed_account : allowed_accounts) {
    if (base::EqualsCaseInsensitiveASCII(account, allowed_account)) {
      return true;
    }
  }
  return false;
}

void EthereumProviderImpl::OnAddEthereumChainRequestCompleted(
    const std::string& chain_id,
    const std::string& error) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (!chain_callbacks_.contains(chain_id_lower) ||
      !chain_ids_.contains(chain_id_lower))
    return;
  if (error.empty()) {
    // To match MM for webcompat, after adding a chain we should prompt
    // again to switch to the chain. And the error result only depends on
    // what the switch action is at that point.
    SwitchEthereumChain(chain_id_lower,
                        std::move(chain_callbacks_[chain_id_lower]),
                        std::move(chain_ids_[chain_id_lower]));
    chain_callbacks_.erase(chain_id_lower);
    chain_ids_.erase(chain_id_lower);
    return;
  }
  bool reject = true;
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest, error);
  std::move(chain_callbacks_[chain_id_lower])
      .Run(std::move(chain_ids_[chain_id_lower]), std::move(formed_response),
           reject, "", false);

  chain_callbacks_.erase(chain_id_lower);
  chain_ids_.erase(chain_id_lower);
}

void EthereumProviderImpl::Request(base::Value input,
                                   RequestCallback callback) {
  CommonRequestOrSendAsync(input, std::move(callback));
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::SendErrorOnRequest(const mojom::ProviderError& error,
                                              const std::string& error_message,
                                              RequestCallback callback,
                                              base::Value id) {
  base::Value formed_response =
      GetProviderErrorDictionary(error, error_message);
  std::move(callback).Run(std::move(id), std::move(formed_response), true, "",
                          false);
}

void EthereumProviderImpl::CommonRequestOrSendAsync(base::ValueView input_value,
                                                    RequestCallback callback) {
  mojom::ProviderError error = mojom::ProviderError::kUnsupportedMethod;
  std::string error_message =
      l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR);
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

  // That check prevents from pop ups from backgrounded pages.
  // We need to add any method that requires a dialog to interact with.
  if ((method == kEthRequestAccounts || method == kAddEthereumChainMethod ||
       method == kSwitchEthereumChainMethod || method == kEthSendTransaction ||
       method == kEthSignTransaction || method == kEthSign ||
       method == kPersonalSign || method == kPersonalEcRecover ||
       method == kEthSignTypedDataV3 || method == kEthSignTypedDataV4 ||
       method == kEthGetEncryptionPublicKey || method == kEthDecrypt ||
       method == kWalletWatchAsset || method == kRequestPermissionsMethod) &&
      !delegate_->IsTabVisible()) {
    SendErrorOnRequest(
        mojom::ProviderError::kResourceUnavailable,
        l10n_util::GetStringUTF8(IDS_WALLET_TAB_IS_NOT_ACTIVE_ERROR),
        std::move(callback), base::Value());
    return;
  }

  if (method == kEthAccounts || method == kEthCoinbase) {
    GetAllowedAccounts(
        false,
        base::BindOnce(&EthereumProviderImpl::OnContinueGetAllowedAccounts,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, delegate_->GetOrigin()));
  } else if (method == kEthRequestAccounts) {
    RequestEthereumPermissions(std::move(callback), std::move(id), method,
                               delegate_->GetOrigin());
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
        base::BindOnce(&EthereumProviderImpl::ContinueGetDefaultKeyringInfo,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), normalized_json_request,
                       delegate_->GetOrigin(), false));
  } else if (method == kEthSignTransaction) {
    json_rpc_service_->GetNetwork(
        mojom::CoinType::ETH,
        base::BindOnce(&EthereumProviderImpl::ContinueGetDefaultKeyringInfo,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), normalized_json_request,
                       delegate_->GetOrigin(), true));
  } else if (method == kEthSendRawTransaction) {
    std::string signed_transaction;
    if (!ParseEthSendRawTransactionParams(normalized_json_request,
                                          &signed_transaction)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    json_rpc_service_->SendRawTransaction(
        signed_transaction,
        base::BindOnce(&EthereumProviderImpl::OnSendRawTransaction,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id)));
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
    base::Value::Dict domain;
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
  } else if (method == kEthGetEncryptionPublicKey) {
    std::string address;
    if (!ParseEthGetEncryptionPublicKeyParams(normalized_json_request,
                                              &address)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    GetEncryptionPublicKey(address, std::move(callback), std::move(id));
  } else if (method == kEthDecrypt) {
    std::string untrusted_encrypted_data_json;
    std::string address;
    if (!ParseEthDecryptParams(normalized_json_request,
                               &untrusted_encrypted_data_json, &address)) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }
    Decrypt(untrusted_encrypted_data_json, address, delegate_->GetOrigin(),
            std::move(callback), std::move(id));
  } else if (method == kWalletWatchAsset || method == kMetamaskWatchAsset) {
    mojom::BlockchainTokenPtr token;
    const auto chain_id = json_rpc_service_->GetChainId(mojom::CoinType::ETH);
    if (!ParseWalletWatchAssetParams(normalized_json_request, chain_id,
                                     mojom::CoinType::ETH, &token,
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
    if (!base::Contains(restricted_methods, "eth_accounts")) {
      SendErrorOnRequest(error, error_message, std::move(callback),
                         std::move(id));
      return;
    }

    RequestEthereumPermissions(std::move(callback), std::move(id), method,
                               delegate_->GetOrigin());
  } else if (method == kGetPermissionsMethod) {
    GetAllowedAccounts(
        true,
        base::BindOnce(&EthereumProviderImpl::OnContinueGetAllowedAccounts,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, delegate_->GetOrigin()));
  } else if (method == kWeb3ClientVersion) {
    Web3ClientVersion(std::move(callback), std::move(id));
  } else {
    json_rpc_service_->Request(normalized_json_request, true, std::move(id),
                               mojom::CoinType::ETH, std::move(callback));
  }
}

void EthereumProviderImpl::Send(const std::string& method,
                                base::Value params,
                                SendCallback callback) {
  CommonRequestOrSendAsync(GetJsonRpcRequest(method, std::move(params)),
                           std::move(callback));
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::RequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin) {
  DCHECK(delegate_);
  if (delegate_->IsPermissionDenied(mojom::CoinType::ETH)) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kNone,
                                 std::vector<std::string>());
    return;
  }
  keyring_service_->GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindOnce(
          &EthereumProviderImpl::ContinueRequestEthereumPermissionsKeyringInfo,
          weak_factory_.GetWeakPtr(), std::move(callback), std::move(id),
          method, origin));
}

void EthereumProviderImpl::ContinueRequestEthereumPermissionsKeyringInfo(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin,
    brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  DCHECK_EQ(keyring_info->id, brave_wallet::mojom::kDefaultKeyringId);
  if (!keyring_info->is_keyring_created) {
    if (!wallet_onboarding_shown_) {
      delegate_->ShowWalletOnboarding();
      wallet_onboarding_shown_ = true;
    }
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kInternal,
                                 absl::nullopt);
    return;
  }

  std::vector<std::string> addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    addresses.push_back(account_info->address);
  }

  if (keyring_info->is_locked) {
    if (pending_request_ethereum_permissions_callback_) {
      OnRequestEthereumPermissions(
          std::move(callback), std::move(id), method, origin,
          RequestPermissionsError::kRequestInProgress, absl::nullopt);
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

  delegate_->GetAllowedAccounts(
      mojom::CoinType::ETH, addresses,
      base::BindOnce(&EthereumProviderImpl::ContinueRequestEthereumPermissions,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(id), method, origin, addresses));
}

void EthereumProviderImpl::ContinueRequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin,
    const std::vector<std::string>& requested_accounts,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!success) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kInternal,
                                 absl::nullopt);
    return;
  }

  if (success && !allowed_accounts.empty()) {
    OnRequestEthereumPermissions(std::move(callback), std::move(id), method,
                                 origin, RequestPermissionsError::kNone,
                                 allowed_accounts);
  } else {
    // Request accounts if no accounts are connected.
    delegate_->RequestPermissions(
        mojom::CoinType::ETH, requested_accounts,
        base::BindOnce(&EthereumProviderImpl::OnRequestEthereumPermissions,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(id), method, origin));
  }
}

void EthereumProviderImpl::Enable(EnableCallback callback) {
  if (!delegate_->IsTabVisible()) {
    SendErrorOnRequest(
        mojom::ProviderError::kResourceUnavailable,
        l10n_util::GetStringUTF8(IDS_WALLET_TAB_IS_NOT_ACTIVE_ERROR),
        std::move(callback), base::Value());
    return;
  }
  RequestEthereumPermissions(std::move(callback), base::Value(), "",
                             delegate_->GetOrigin());
  delegate_->WalletInteractionDetected();
}

void EthereumProviderImpl::OnRequestEthereumPermissions(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin,
    RequestPermissionsError error,
    const absl::optional<std::vector<std::string>>& allowed_accounts) {
  base::Value formed_response;

  bool success = error == RequestPermissionsError::kNone;
  std::vector<std::string> accounts;
  if (success && allowed_accounts) {
    accounts = FilterAccounts(
        *allowed_accounts,
        keyring_service_->GetSelectedAccount(mojom::CoinType::ETH));
  }

  std::string first_allowed_account;
  if (accounts.size() > 0) {
    first_allowed_account = base::ToLowerASCII(accounts[0]);
  }
  if (success && accounts.empty()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  } else if (!success) {
    switch (error) {
      case RequestPermissionsError::kRequestInProgress:
        formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kUserRejectedRequest,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
        break;
      case RequestPermissionsError::kInternal:
        formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
        break;
      default:
        NOTREACHED();
    }
  } else if (method == kRequestPermissionsMethod) {
    formed_response =
        base::Value(PermissionRequestResponseToValue(origin, accounts));
  } else {
    base::Value::List list;
    for (const auto& account : accounts) {
      list.Append(base::ToLowerASCII(account));
    }
    formed_response = base::Value(std::move(list));
  }
  bool reject = !success || accounts.empty();

  std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                          first_allowed_account, true);
}

void EthereumProviderImpl::GetAllowedAccounts(
    bool include_accounts_when_locked,
    GetAllowedAccountsCallback callback) {
  keyring_service_->GetKeyringInfo(
      brave_wallet::mojom::kDefaultKeyringId,
      base::BindOnce(&EthereumProviderImpl::ContinueGetAllowedAccounts,
                     weak_factory_.GetWeakPtr(), include_accounts_when_locked,
                     std::move(callback)));
}

void EthereumProviderImpl::ContinueGetAllowedAccounts(
    bool include_accounts_when_locked,
    GetAllowedAccountsCallback callback,
    brave_wallet::mojom::KeyringInfoPtr keyring_info) {
  std::vector<std::string> addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    addresses.push_back(base::ToLowerASCII(account_info->address));
  }

  DCHECK(delegate_);
  delegate_->GetAllowedAccounts(
      mojom::CoinType::ETH, addresses,
      base::BindOnce(&EthereumProviderImpl::OnGetAllowedAccounts,
                     weak_factory_.GetWeakPtr(), include_accounts_when_locked,
                     keyring_info->is_locked,
                     keyring_service_->GetSelectedAccount(mojom::CoinType::ETH),
                     std::move(callback)));
}

void EthereumProviderImpl::OnGetAllowedAccounts(
    bool include_accounts_when_locked,
    bool keyring_locked,
    const absl::optional<std::string>& selected_account,
    GetAllowedAccountsCallback callback,
    bool success,
    const std::vector<std::string>& accounts) {
  std::vector<std::string> filtered_accounts;
  if (!keyring_locked || include_accounts_when_locked) {
    filtered_accounts = FilterAccounts(accounts, selected_account);
  }

  std::move(callback).Run(
      filtered_accounts,
      success ? mojom::ProviderError::kSuccess
              : mojom::ProviderError::kInternalError,
      success ? "" : l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

void EthereumProviderImpl::OnContinueGetAllowedAccounts(
    RequestCallback callback,
    base::Value id,
    const std::string& method,
    const url::Origin& origin,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  bool reject = error != mojom::ProviderError::kSuccess;
  bool update_bindings = false;
  base::Value formed_response;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else if (method == kEthAccounts) {
    base::Value::List list;
    for (const auto& account : accounts) {
      list.Append(base::ToLowerASCII(account));
    }
    formed_response = base::Value(std::move(list));
    update_bindings = false;
  } else if (method == kEthCoinbase) {
    if (accounts.empty()) {
      formed_response = base::Value();
    } else {
      formed_response = base::Value(base::ToLowerASCII(accounts[0]));
    }
    update_bindings = false;
  } else {
    formed_response =
        base::Value(PermissionRequestResponseToValue(origin, accounts));
    update_bindings = true;
  }
  std::move(callback).Run(std::move(id), std::move(formed_response), reject, "",
                          update_bindings);
}

void EthereumProviderImpl::UpdateKnownAccounts() {
  GetAllowedAccounts(
      false, base::BindOnce(&EthereumProviderImpl::OnUpdateKnownAccounts,
                            weak_factory_.GetWeakPtr()));
}

void EthereumProviderImpl::OnUpdateKnownAccounts(
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

void EthereumProviderImpl::Web3ClientVersion(RequestCallback callback,
                                             base::Value id) {
  std::move(callback).Run(std::move(id), base::Value(GetWeb3ClientVersion()),
                          false, "", false);
}

void EthereumProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (json_rpc_service_) {
    json_rpc_service_->GetChainId(mojom::CoinType::ETH, std::move(callback));
  }
}

void EthereumProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void EthereumProviderImpl::ChainChangedEvent(const std::string& chain_id,
                                             mojom::CoinType coin) {
  if (!events_listener_.is_bound() || coin != mojom::CoinType::ETH)
    return;

  events_listener_->ChainChangedEvent(chain_id);
}

void EthereumProviderImpl::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  auto tx_status = tx_info->tx_status;
  if (tx_status != mojom::TransactionStatus::Submitted &&
      tx_status != mojom::TransactionStatus::Signed &&
      tx_status != mojom::TransactionStatus::Rejected &&
      tx_status != mojom::TransactionStatus::Error)
    return;

  std::string tx_meta_id = tx_info->id;
  if (!add_tx_callbacks_.contains(tx_meta_id) ||
      !add_tx_ids_.contains(tx_meta_id))
    return;

  std::string tx_hash = tx_info->tx_hash;
  base::Value formed_response;
  bool reject = true;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    formed_response = base::Value(tx_hash);
    reject = false;
  } else if (tx_status == mojom::TransactionStatus::Signed) {
    std::string signed_transaction;
    if (tx_info->tx_data_union->is_eth_tx_data()) {
      DCHECK(tx_info->tx_data_union->get_eth_tx_data()->signed_transaction);
      signed_transaction =
          *tx_info->tx_data_union->get_eth_tx_data()->signed_transaction;
    } else if (tx_info->tx_data_union->is_eth_tx_data_1559()) {
      DCHECK(tx_info->tx_data_union->get_eth_tx_data_1559()
                 ->base_data->signed_transaction);
      signed_transaction = *tx_info->tx_data_union->get_eth_tx_data_1559()
                                ->base_data->signed_transaction;
    }
    formed_response = base::Value(signed_transaction);
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
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_ERROR));
    reject = true;
  }
  std::move(add_tx_callbacks_[tx_meta_id])
      .Run(std::move(add_tx_ids_[tx_meta_id]), std::move(formed_response),
           reject, "", false);
  add_tx_callbacks_.erase(tx_meta_id);
  add_tx_ids_.erase(tx_meta_id);
}

void EthereumProviderImpl::SelectedAccountChanged(mojom::CoinType coin) {
  if (coin != mojom::CoinType::ETH)
    return;
  UpdateKnownAccounts();
}

void EthereumProviderImpl::Locked() {
  UpdateKnownAccounts();
}

void EthereumProviderImpl::Unlocked() {
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

void EthereumProviderImpl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::BRAVE_ETHEREUM) {
    UpdateKnownAccounts();
  }
}

void EthereumProviderImpl::AddSuggestToken(mojom::BlockchainTokenPtr token,
                                           RequestCallback callback,
                                           base::Value id) {
  if (!token) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  auto request = mojom::AddSuggestTokenRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), std::move(token));
  brave_wallet_service_->AddSuggestTokenRequest(
      std::move(request), std::move(callback), std::move(id));
  delegate_->ShowPanel();
}

void EthereumProviderImpl::OnSendRawTransaction(
    RequestCallback callback,
    base::Value id,
    const std::string& tx_hash,
    mojom::ProviderError error,
    const std::string& error_message) {
  base::Value formed_response;
  if (error != mojom::ProviderError::kSuccess) {
    formed_response = GetProviderErrorDictionary(error, error_message);
  } else {
    formed_response = base::Value(tx_hash);
  }
  std::move(callback).Run(std::move(id), std::move(formed_response),
                          error != mojom::ProviderError::kSuccess, "", false);
}

}  // namespace brave_wallet
