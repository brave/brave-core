/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <utility>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void RespondForEthereumChainRequest(
    brave_wallet::BraveWalletProviderImpl::AddEthereumChainCallback callback,
    const std::string& error) {
  base::flat_map<std::string, std::string> headers;
  if (!error.empty()) {
    auto response = base::StringPrintf(
        brave_wallet::kJsonResponseAddEthereumChainError, error.c_str());
    std::move(callback).Run(200, response, headers);
  } else {
    std::move(callback).Run(
        200, brave_wallet::kJsonResponseAddEthereumChainSuccess, headers);
  }
}

}  // namespace

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    PrefService* prefs)
    : delegate_(std::move(delegate)), prefs_(prefs), weak_factory_(this) {
  DCHECK(rpc_controller);
  rpc_controller_.Bind(std::move(rpc_controller));
  DCHECK(rpc_controller_);
  rpc_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {}

void BraveWalletProviderImpl::AddEthereumChain(
    std::vector<mojom::EthereumChainPtr> chains,
    AddEthereumChainCallback callback) {
  if (!delegate_ || chains.empty()) {
    RespondForEthereumChainRequest(
        std::move(callback),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  DCHECK_LT(chains.size(), size_t(2));
  const auto& chain = chains.front();
  if (GetNetworkURL(prefs_, chain->chain_id).is_valid()) {
    RespondForEthereumChainRequest(
        std::move(callback), l10n_util::GetStringUTF8(IDS_WALLET_CHAIN_EXISTS));
    return;
  }
  // By https://eips.ethereum.org/EIPS/eip-3085 only chain id is required
  // we expect chain name and rpc urls as well at this time
  // https://github.com/brave/brave-browser/issues/17637
  if (chain->chain_id.empty() || chain->rpc_urls.empty() ||
      chain->chain_name.empty()) {
    RespondForEthereumChainRequest(
        std::move(callback),
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  if (chain_callbacks_.contains(chain->chain_id)) {
    RespondForEthereumChainRequest(
        std::move(callback),
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }
  auto value = brave_wallet::EthereumChainToValue(chain);
  std::string serialized_chain;
  if (!base::JSONWriter::Write(value, &serialized_chain)) {
    RespondForEthereumChainRequest(
        std::move(callback),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  chain_callbacks_[chain->chain_id] = std::move(callback);
  delegate_->RequestUserApproval(
      chain->chain_id, serialized_chain,
      base::BindOnce(&BraveWalletProviderImpl::OnChainApprovalResult,
                     weak_factory_.GetWeakPtr(), chain->chain_id));
  return;
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
  observer_receiver_.reset();
}

void BraveWalletProviderImpl::OnChainApprovalResult(const std::string& chain_id,
                                                    const std::string& error) {
  DCHECK(chain_callbacks_.contains(chain_id));
  RespondForEthereumChainRequest(std::move(chain_callbacks_[chain_id]), error);
  chain_callbacks_.erase(chain_id);
}

}  // namespace brave_wallet
