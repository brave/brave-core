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
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    HostContentSettingsMap* host_content_settings_map,
    mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
    mojo::PendingRemote<mojom::EthTxController> tx_controller,
    KeyringController* keyring_controller,
    BraveWalletService* brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map),
      delegate_(std::move(delegate)),
      keyring_controller_(keyring_controller),
      brave_wallet_service_(brave_wallet_service),
      prefs_(prefs),
      weak_factory_(this) {
  DCHECK(rpc_controller);
  rpc_controller_.Bind(std::move(rpc_controller));
  DCHECK(rpc_controller_);
  rpc_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
  rpc_controller_->AddObserver(
      rpc_observer_receiver_.BindNewPipeAndPassRemote());

  DCHECK(tx_controller);
  tx_controller_.Bind(std::move(tx_controller));
  tx_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
  tx_controller_->AddObserver(tx_observer_receiver_.BindNewPipeAndPassRemote());

  keyring_controller_->AddObserver(
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
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto json_value = base::JSONReader::Read(json_payload);
  if (!json_value) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  const base::Value* params = json_value->FindListPath(brave_wallet::kParams);
  if (!params || !params->is_list()) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const auto list = params->GetList();
  if (list.empty()) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_EXPECTED_SINGLE_PARAMETER));
    return;
  }
  auto chain = brave_wallet::ValueToEthereumChain(list.front());
  if (!chain) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (GetNetworkURL(prefs_, chain->chain_id).is_valid()) {
    std::move(callback).Run(true, 0, std::string());
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain->chain_id.empty() || chain->rpc_urls.empty() ||
      chain->chain_name.empty()) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  if (chain_callbacks_.contains(chain->chain_id)) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kUserRejectedRequest),
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }
  if (!delegate_) {
    std::move(callback).Run(
        false, static_cast<int>(ProviderErrors::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  chain_callbacks_[chain->chain_id] = std::move(callback);
  rpc_controller_->AddEthereumChain(
      chain->Clone(), delegate_->GetOrigin(),
      base::BindOnce(&BraveWalletProviderImpl::OnAddEthereumChain,
                     base::Unretained(this)));
}

void BraveWalletProviderImpl::OnAddEthereumChain(const std::string& chain_id,
                                                 bool accepted) {
  DCHECK(delegate_);
  if (!chain_callbacks_.contains(chain_id))
    return;
  if (!accepted) {
    std::move(chain_callbacks_[chain_id])
        .Run(false, static_cast<int>(ProviderErrors::kUserRejectedRequest),
             l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));

    chain_callbacks_.erase(chain_id);
    return;
  }
  delegate_->ShowBubble();
}

void BraveWalletProviderImpl::AddAndApproveTransaction(
    mojom::TxDataPtr tx_data,
    const std::string& from,
    AddAndApproveTransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  GetAllowedAccounts(
      base::BindOnce(&BraveWalletProviderImpl::ContinueAddAndApproveTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(tx_data), from));
}

void BraveWalletProviderImpl::ContinueAddAndApproveTransaction(
    AddAndApproveTransactionCallback callback,
    mojom::TxDataPtr tx_data,
    const std::string& from,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!CheckAccountAllowed(from, allowed_accounts)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    return;
  }

  tx_controller_->AddUnapprovedTransaction(
      std::move(tx_data), from,
      base::BindOnce(&BraveWalletProviderImpl::OnAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::AddAndApprove1559Transaction(
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    AddAndApprove1559TransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  GetAllowedAccounts(base::BindOnce(
      &BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction,
      weak_factory_.GetWeakPtr(), std::move(callback), std::move(tx_data),
      from));
}

void BraveWalletProviderImpl::ContinueAddAndApprove1559Transaction(
    AddAndApprove1559TransactionCallback callback,
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!CheckAccountAllowed(from, allowed_accounts)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(
            IDS_WALLET_ETH_SEND_TRANSACTION_FROM_NOT_AUTHED));
    return;
  }

  tx_controller_->AddUnapproved1559Transaction(
      std::move(tx_data), from,
      base::BindOnce(&BraveWalletProviderImpl::OnAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnAddUnapprovedTransaction(
    AddAndApproveTransactionCallback callback,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  if (success) {
    add_tx_callbacks_[tx_meta_id] = std::move(callback);
    delegate_->ShowBubble();
  } else {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_ERROR));
  }
}

void BraveWalletProviderImpl::SignMessage(const std::string& address,
                                          const std::string& message,
                                          SignMessageCallback callback) {
  if (!EthAddress::IsValidAddress(address) || !IsValidHexString(message)) {
    std::move(callback).Run(
        "", static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::vector<uint8_t> message_bytes;
  if (!base::HexStringToBytes(message.substr(2), &message_bytes)) {
    std::move(callback).Run(
        "", static_cast<int>(ProviderErrors::kInvalidParams),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  // Convert to checksum address
  auto checksum_address = EthAddress::FromHex(address);
  GetAllowedAccounts(base::BindOnce(
      &BraveWalletProviderImpl::ContinueSignMessage, weak_factory_.GetWeakPtr(),
      checksum_address.ToChecksumAddress(), std::move(message_bytes),
      std::move(callback)));
}

void BraveWalletProviderImpl::ContinueSignMessage(
    const std::string& address,
    std::vector<uint8_t>&& message,
    SignMessageCallback callback,
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!CheckAccountAllowed(address, allowed_accounts)) {
    std::move(callback).Run(
        "", static_cast<int>(ProviderErrors::kUnauthorized),
        l10n_util::GetStringFUTF8(IDS_WALLET_ETH_SIGN_NOT_AUTHED,
                                  base::ASCIIToUTF16(address)));
    return;
  }

  std::string message_to_request = std::string(message.begin(), message.end());
  brave_wallet_service_->AddSignMessageRequest(
      {sign_message_id_++, address, std::move(message_to_request)},
      base::BindOnce(&BraveWalletProviderImpl::OnSignMessageRequestProcessed,
                     weak_factory_.GetWeakPtr(), std::move(callback), address,
                     std::move(message)));
  delegate_->ShowBubble();
}

void BraveWalletProviderImpl::OnSignMessageRequestProcessed(
    SignMessageCallback callback,
    const std::string& address,
    std::vector<uint8_t>&& message,
    bool approved) {
  if (!approved) {
    std::move(callback).Run(
        "", static_cast<int>(ProviderErrors::kUserRejectedRequest),
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    return;
  }
  auto signature_with_err =
      keyring_controller_->SignMessageByDefaultKeyring(address, message);
  if (!signature_with_err.signature)
    std::move(callback).Run("",
                            static_cast<int>(ProviderErrors::kInternalError),
                            signature_with_err.error_message);
  else
    std::move(callback).Run(ToHex(*signature_with_err.signature), 0, "");
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
    std::move(chain_callbacks_[chain_id]).Run(true, 0, std::string());
  } else {
    std::move(chain_callbacks_[chain_id])
        .Run(false, static_cast<int>(ProviderErrors::kUserRejectedRequest),
             error);
  }
  chain_callbacks_.erase(chain_id);
}

void BraveWalletProviderImpl::Request(const std::string& json_payload,
                                      bool auto_retry_on_network_change,
                                      RequestCallback callback) {
  if (rpc_controller_) {
    rpc_controller_->Request(json_payload, true, std::move(callback));
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
    bool success,
    const std::vector<std::string>& accounts) {
  std::move(callback).Run(success, accounts);
}

void BraveWalletProviderImpl::GetAllowedAccounts(
    GetAllowedAccountsCallback callback) {
  DCHECK(delegate_);
  delegate_->GetAllowedAccounts(
      base::BindOnce(&BraveWalletProviderImpl::OnGetAllowedAccounts,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnGetAllowedAccounts(
    GetAllowedAccountsCallback callback,
    bool success,
    const std::vector<std::string>& accounts) {
  std::move(callback).Run(success, accounts);
}

void BraveWalletProviderImpl::UpdateKnownAccounts() {
  GetAllowedAccounts(
      base::BindOnce(&BraveWalletProviderImpl::OnUpdateKnownAccounts,
                     weak_factory_.GetWeakPtr()));
}

void BraveWalletProviderImpl::OnUpdateKnownAccounts(
    bool success,
    const std::vector<std::string>& allowed_accounts) {
  if (!success) {
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
  if (rpc_controller_) {
    rpc_controller_->GetChainId(std::move(callback));
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
  rpc_controller_.reset();
  tx_controller_.reset();
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
    std::move(add_tx_callbacks_[tx_meta_id]).Run(true, tx_hash, "");
  } else if (tx_status == mojom::TransactionStatus::Rejected) {
    std::move(add_tx_callbacks_[tx_meta_id])
        .Run(false, "",
             l10n_util::GetStringUTF8(
                 IDS_WALLET_ETH_SEND_TRANSACTION_USER_REJECTED));
  } else if (tx_status == mojom::TransactionStatus::Error) {
    std::move(add_tx_callbacks_[tx_meta_id])
        .Run(false, "",
             l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_ERROR));
  }
  add_tx_callbacks_.erase(tx_meta_id);
}

void BraveWalletProviderImpl::SelectedAccountChanged() {
  UpdateKnownAccounts();
}

void BraveWalletProviderImpl::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  if (content_type == ContentSettingsType::BRAVE_ETHEREUM) {
    UpdateKnownAccounts();
  }
}

}  // namespace brave_wallet
