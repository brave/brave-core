/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
    mojo::PendingRemote<mojom::EthTxController> tx_controller,
    mojo::PendingRemote<mojom::TrezorBridgeController> trezor_controller,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : delegate_(std::move(delegate)), prefs_(prefs), weak_factory_(this) {
  DCHECK(rpc_controller);
  rpc_controller_.Bind(std::move(rpc_controller));
  DCHECK(rpc_controller_);
  rpc_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
  trezor_controller_.Bind(std::move(trezor_controller));
  trezor_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));

  DCHECK(tx_controller);
  tx_controller_.Bind(std::move(tx_controller));
  tx_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {}

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

void BraveWalletProviderImpl::AddUnapprovedTransaction(
    mojom::TxDataPtr tx_data,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  GetAllowedAccounts(
      base::BindOnce(&BraveWalletProviderImpl::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     std::move(tx_data), from));
}

void BraveWalletProviderImpl::ContinueAddUnapprovedTransaction(
    AddUnapprovedTransactionCallback callback,
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

void BraveWalletProviderImpl::AddUnapproved1559Transaction(
    mojom::TxData1559Ptr tx_data,
    const std::string& from,
    AddUnapproved1559TransactionCallback callback) {
  if (!tx_data) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_ETH_SEND_TRANSACTION_NO_TX_DATA));
    return;
  }

  GetAllowedAccounts(base::BindOnce(
      &BraveWalletProviderImpl::ContinueAddUnapproved1559Transaction,
      weak_factory_.GetWeakPtr(), std::move(callback), std::move(tx_data),
      from));
}

void BraveWalletProviderImpl::ContinueAddUnapproved1559Transaction(
    AddUnapproved1559TransactionCallback callback,
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
      base::BindOnce(&BraveWalletProviderImpl::OnAddUnapproved1559Transaction,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveWalletProviderImpl::OnAddUnapprovedTransaction(
    AddUnapprovedTransactionCallback callback,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  std::move(callback).Run(success, tx_meta_id, error_message);
  if (success) {
    delegate_->ShowBubble();
  }
}

void BraveWalletProviderImpl::OnAddUnapproved1559Transaction(
    AddUnapproved1559TransactionCallback callback,
    bool success,
    const std::string& tx_meta_id,
    const std::string& error_message) {
  std::move(callback).Run(success, tx_meta_id, error_message);
  if (success) {
    delegate_->ShowBubble();
  }
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

void BraveWalletProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (rpc_controller_) {
    rpc_controller_->GetChainId(std::move(callback));
  }
}

void BraveWalletProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
    if (rpc_controller_) {
      rpc_controller_->AddObserver(
          observer_receiver_.BindNewPipeAndPassRemote());
    }
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
  trezor_controller_.reset();
  observer_receiver_.reset();
}

}  // namespace brave_wallet
