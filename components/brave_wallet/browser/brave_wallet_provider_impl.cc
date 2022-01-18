/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

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
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    HostContentSettingsMap* host_content_settings_map,
    JsonRpcService* json_rpc_service,
    mojo::PendingRemote<mojom::EthTxService> tx_service,
    KeyringService* keyring_service,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      brave_wallet_service_(brave_wallet_service),
      prefs_(prefs),
      weak_factory_(this) {
  DCHECK(json_rpc_service);
  json_rpc_service_->AddObserver(
      rpc_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_service);
  tx_service_.Bind(std::move(tx_service));
  tx_service_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
  tx_service_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());

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

void BraveWalletProviderImpl::AddEthereumChain(
    const std::string& json_payload,
    AddEthereumChainCallback callback) {
  if (json_payload.empty()) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto json_value = base::JSONReader::Read(
      json_payload,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  if (!json_value) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  const base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list()) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const auto list = params->GetList();
  if (list.empty()) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_EXPECTED_SINGLE_PARAMETER));
    return;
  }
  auto chain = brave_wallet::ValueToEthereumChain(list.front());
  if (!chain) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  // Check if we already have the chain
  if (GetNetworkURL(prefs_, chain->chain_id).is_valid()) {
    if (json_rpc_service_->GetChainId() != chain->chain_id) {
      SwitchEthereumChain(chain->chain_id, std::move(callback));
      return;
    }

    std::move(callback).Run(mojom::ProviderError::kSuccess, std::string());
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain->chain_id.empty() || chain->rpc_urls.empty() ||
      chain->chain_name.empty()) {
    std::move(callback).Run(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  if (chain_callbacks_.contains(chain->chain_id)) {
    std::move(callback).Run(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }
  if (!delegate_) {
    std::move(callback).Run(
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  chain_callbacks_[chain->chain_id] = std::move(callback);
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
  if (!chain_callbacks_.contains(chain_id))
    return;
  if (error != mojom::ProviderError::kSuccess) {
    std::move(chain_callbacks_[chain_id]).Run(error, error_message);
    chain_callbacks_.erase(chain_id);
    return;
  }
  delegate_->ShowPanel();
}

void BraveWalletProviderImpl::SwitchEthereumChain(
    const std::string& chain_id,
    SwitchEthereumChainCallback callback) {
  // Only show bubble when there is no immediate error
  if (json_rpc_service_->AddSwitchEthereumChainRequest(
          chain_id, delegate_->GetOrigin(), std::move(callback)))
    delegate_->ShowPanel();
}

void BraveWalletProviderImpl::GetNetworkAndDefaultKeyringInfo(
    GetNetworkAndDefaultKeyringInfoCallback callback) {
  json_rpc_service_->GetNetwork(
      base::BindOnce(&BraveWalletProviderImpl::ContinueGetDefaultKeyringInfo,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::ContinueGetDefaultKeyringInfo(
    GetNetworkAndDefaultKeyringInfoCallback callback,
    mojom::EthereumChainPtr chain) {
  keyring_service_->GetKeyringInfo(
      mojom::kDefaultKeyringId,
      base::BindOnce(
          &BraveWalletProviderImpl::OnGetNetworkAndDefaultKeyringInfo,
          weak_factory_.GetWeakPtr(), std::move(callback), std::move(chain)));
}

void BraveWalletProviderImpl::OnGetNetworkAndDefaultKeyringInfo(
    GetNetworkAndDefaultKeyringInfoCallback callback,
    mojom::EthereumChainPtr chain,
    mojom::KeyringInfoPtr keyring_info) {
  std::move(callback).Run(std::move(chain), std::move(keyring_info));
}

void BraveWalletProviderImpl::IsLocked(IsLockedCallback callback) {
  keyring_service_->IsLocked(std::move(callback));
}

void BraveWalletProviderImpl::AddAndApproveTransaction(
    mojom::TxDataPtr tx_data,
    const std::string& from,
    AddAndApproveTransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        "", brave_wallet::mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  GetAllowedAccounts(
      false,
      base::BindOnce(&BraveWalletProviderImpl::ContinueAddAndApproveTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(tx_data), from));
}

void BraveWalletProviderImpl::ContinueAddAndApproveTransaction(
    AddAndApproveTransactionCallback callback,
    mojom::TxDataPtr tx_data,
    const std::string& from,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    return;
  }

  tx_service_->AddUnapprovedTransaction(
      std::move(tx_data), from,
      base::BindOnce(
          &BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter,
          weak_factory_.GetWeakPtr(), std::move(callback)));
}

// AddUnapprovedTransaction is a different return type from
// AddAndApproveTransaction so we need to use an adapter callback that passses
// through.
void BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter(
    AddAndApproveTransactionCallback callback,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  OnAddUnapprovedTransaction(std::move(callback), tx_meta_id,
                             success ? mojom::ProviderError::kSuccess
                                     : mojom::ProviderError::kInternalError,
                             success ? "" : error_message);
}

void BraveWalletProviderImpl::AddAndApprove1559Transaction(
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    AddAndApprove1559TransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        "", brave_wallet::mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  // If the chain id is not known yet, then get it and set it first
  if (tx_data->chain_id == "0x0" || tx_data->chain_id.empty()) {
    json_rpc_service_->GetChainId(base::BindOnce(
        &BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction,
        weak_factory_.GetWeakPtr(), std::move(callback), std::move(tx_data),
        from));
  } else {
    GetAllowedAccounts(
        false,
        base::BindOnce(&BraveWalletProviderImpl::
                           ContinueAddAndApprove1559TransactionWithAccounts,
                       weak_factory_.GetWeakPtr(), std::move(callback),
                       std::move(tx_data), from));
  }
}

void BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction(
    AddAndApprove1559TransactionCallback callback,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const std::string& chain_id) {
  tx_data->chain_id = chain_id;
  GetAllowedAccounts(
      false,
      base::BindOnce(&BraveWalletProviderImpl::
                         ContinueAddAndApprove1559TransactionWithAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(tx_data), from));
}

void BraveWalletProviderImpl::ContinueAddAndApprove1559TransactionWithAccounts(
    AddAndApprove1559TransactionCallback callback,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  if (!CheckAccountAllowed(from, allowed_accounts)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    return;
  }

  tx_service_->AddUnapproved1559Transaction(
      std::move(tx_data), from,
      base::BindOnce(
          &BraveWalletProviderImpl::OnAddUnapprovedTransactionAdapter,
          weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnAddUnapprovedTransaction(
    AddAndApproveTransactionCallback callback,
    const std::string& tx_meta_id,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error == mojom::ProviderError::kSuccess) {
    add_tx_callbacks_[tx_meta_id] = std::move(callback);
    delegate_->ShowPanel();
  } else {
    std::move(callback).Run("", error, error_message);
  }
}

void BraveWalletProviderImpl::SignMessage(const std::string& address,
                                          const std::string& message,
                                          SignMessageCallback callback) {
  if (!EthAddress::IsValidAddress(address) || !IsValidHexString(message)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
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
                     std::move(message_bytes), std::move(callback), false));
}

void BraveWalletProviderImpl::RecoverAddress(const std::string& message,
                                             const std::string& signature,
                                             RecoverAddressCallback callback) {
  // 65 * 2 hex chars per byte + 2 chars for  0x
  if (signature.length() != 132) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!PrefixedHexStringToBytes(message, &message_bytes)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::vector<uint8_t> signature_bytes;
  if (!PrefixedHexStringToBytes(signature, &signature_bytes)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string address;
  if (!keyring_service_->RecoverAddressByDefaultKeyring(
          message_bytes, signature_bytes, &address)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(address, mojom::ProviderError::kSuccess, "");
}

void BraveWalletProviderImpl::SignTypedMessage(
    const std::string& address,
    const std::string& message,
    const std::string& message_to_sign,
    base::Value domain,
    SignTypedMessageCallback callback) {
  std::vector<uint8_t> eip712_hash;
  if (!EthAddress::IsValidAddress(address) ||
      !base::HexStringToBytes(message_to_sign, &eip712_hash) ||
      eip712_hash.size() != 32 || !domain.is_dict()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto chain_id = domain.FindDoubleKey("chainId");
  if (chain_id) {
    const std::string chain_id_hex =
        Uint256ValueToHex((uint256_t)(uint64_t)*chain_id);
    if (chain_id_hex != json_rpc_service_->GetChainId()) {
      std::move(callback).Run(
          "", mojom::ProviderError::kInternalError,
          l10n_util::GetStringFUTF8(
              IDS_BRAVE_WALLET_SIGN_TYPED_MESSAGE_CHAIN_ID_MISMATCH,
              base::ASCIIToUTF16(chain_id_hex)));
      return;
    }
  }

  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(
      false, base::BindOnce(&BraveWalletProviderImpl::ContinueSignMessage,
                            weak_factory_.GetWeakPtr(),
                            checksum_address.ToChecksumAddress(), message,
                            std::move(eip712_hash), std::move(callback), true));
}

void BraveWalletProviderImpl::ContinueSignMessage(
    const std::string& address,
    const std::string& message,
    std::vector<uint8_t>&& message_to_sign,
    SignMessageCallback callback,
    bool is_eip712,
    const std::vector<std::string>& allowed_accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  if (!CheckAccountAllowed(address, allowed_accounts)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kUnauthorized,
        l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                  base::ASCIIToUTF16(address)));
    return;
  }

  auto request =
      mojom::SignMessageRequest::New(sign_message_id_++, address, message);
  if (keyring_service_->IsHardwareAccount(address)) {
    brave_wallet_service_->AddSignMessageRequest(
        std::move(request),
        base::BindOnce(
            &BraveWalletProviderImpl::OnHardwareSignMessageRequestProcessed,
            weak_factory_.GetWeakPtr(), std::move(callback), address,
            std::move(message_to_sign), is_eip712));
  } else {
    brave_wallet_service_->AddSignMessageRequest(
        std::move(request),
        base::BindOnce(&BraveWalletProviderImpl::OnSignMessageRequestProcessed,
                       weak_factory_.GetWeakPtr(), std::move(callback), address,
                       std::move(message_to_sign), is_eip712));
  }
  delegate_->ShowPanel();
}

void BraveWalletProviderImpl::OnSignMessageRequestProcessed(
    SignMessageCallback callback,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool is_eip712,
    bool approved,
    const std::string& signature,
    const std::string& error) {
  if (!approved) {
    std::move(callback).Run(
        "", mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    return;
  }

  auto signature_with_err = keyring_service_->SignMessageByDefaultKeyring(
      address, message, is_eip712);
  if (!signature_with_err.signature)
    std::move(callback).Run("", mojom::ProviderError::kInternalError,
                            signature_with_err.error_message);
  else
    std::move(callback).Run(ToHex(*signature_with_err.signature),
                            mojom::ProviderError::kSuccess, "");
}

void BraveWalletProviderImpl::OnHardwareSignMessageRequestProcessed(
    SignMessageCallback callback,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool is_eip712,
    bool approved,
    const std::string& signature,
    const std::string& error) {
  if (!approved) {
    mojom::ProviderError error_code =
        error.empty() ? mojom::ProviderError::kUserRejectedRequest
                      : mojom::ProviderError::kInternalError;
    auto error_message =
        error.empty()
            ? l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST)
            : error;
    std::move(callback).Run("", error_code, error_message);
    return;
  }

  std::move(callback).Run(signature, mojom::ProviderError::kSuccess, "");
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
  if (!chain_callbacks_.contains(chain_id))
    return;
  if (error.empty()) {
    // To match MM for webcompat, after adding a chain we should prompt
    // again to switch to the chain. And the error result only depends on
    // what the switch action is at that point.
    SwitchEthereumChain(chain_id, std::move(chain_callbacks_[chain_id]));
    chain_callbacks_.erase(chain_id);
    return;
  }
  std::move(chain_callbacks_[chain_id])
      .Run(mojom::ProviderError::kUserRejectedRequest, error);
  chain_callbacks_.erase(chain_id);
}

void BraveWalletProviderImpl::Request(const std::string& json_payload,
                                      bool auto_retry_on_network_change,
                                      RequestCallback callback) {
  if (json_rpc_service_) {
    json_rpc_service_->Request(json_payload, true, std::move(callback));
  }
}

void BraveWalletProviderImpl::RequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback) {
  DCHECK(delegate_);
  delegate_->RequestEthereumPermissions(
      base::BindOnce(&BraveWalletProviderImpl::OnRequestEthereumPermissions,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnRequestEthereumPermissions(
    RequestEthereumPermissionsCallback callback,
    const std::vector<std::string>& accounts,
    mojom::ProviderError error,
    const std::string& error_message) {
  // If the call was successful but the keyring is locked, then request an
  // unlock.  After the unlock happens a new request will be made.
  if (error == mojom::ProviderError::kSuccess && keyring_service_->IsLocked()) {
    if (pending_request_ethereum_permissions_callback_) {
      std::move(callback).Run(
          std::vector<std::string>(),
          mojom::ProviderError::kUserRejectedRequest,
          l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
      return;
    }
    pending_request_ethereum_permissions_callback_ = std::move(callback);
    keyring_service_->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  std::move(callback).Run(accounts, error, error_message);
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
    json_rpc_service_->GetChainId(std::move(callback));
  }
}

void BraveWalletProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void BraveWalletProviderImpl::ChainChangedEvent(const std::string& chain_id) {
  if (!events_listener_.is_bound())
    return;

  events_listener_->ChainChangedEvent(chain_id);
}

void BraveWalletProviderImpl::OnConnectionError() {
  tx_service_.reset();
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
  if (!add_tx_callbacks_.contains(tx_meta_id))
    return;

  std::string tx_hash = tx_info->tx_hash;
  if (tx_status == mojom::TransactionStatus::Submitted) {
    std::move(add_tx_callbacks_[tx_meta_id])
        .Run(tx_hash, mojom::ProviderError::kSuccess, "");
  } else if (tx_status == mojom::TransactionStatus::Rejected) {
    std::move(add_tx_callbacks_[tx_meta_id])
        .Run("", mojom::ProviderError::kUserRejectedRequest,
             l10n_util::GetStringUTF8(
                 IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
  } else if (tx_status == mojom::TransactionStatus::Error) {
    std::move(add_tx_callbacks_[tx_meta_id])
        .Run("", mojom::ProviderError::kInternalError,
             l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_ERROR));
  }
  add_tx_callbacks_.erase(tx_meta_id);
}

void BraveWalletProviderImpl::SelectedAccountChanged() {
  UpdateKnownAccounts();
}

void BraveWalletProviderImpl::Locked() {
  UpdateKnownAccounts();
}

void BraveWalletProviderImpl::Unlocked() {
  if (pending_request_ethereum_permissions_callback_) {
    RequestEthereumPermissions(
        std::move(pending_request_ethereum_permissions_callback_));
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

void BraveWalletProviderImpl::AddSuggestToken(
    mojom::BlockchainTokenPtr token,
    AddSuggestTokenCallback callback) {
  if (!token) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto request = mojom::AddSuggestTokenRequest::New(std::move(token));
  brave_wallet_service_->AddSuggestTokenRequest(std::move(request),
                                                std::move(callback));
  delegate_->ShowPanel();
}

}  // namespace brave_wallet
