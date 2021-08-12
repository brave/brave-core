/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"

namespace {
const char kAddEthereumChainMethod[] = "wallet_addEthereumChain";
}  // namespace

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
    std::unique_ptr<BraveWalletProviderDelegate> delegate)
    : delegate_(std::move(delegate)), weak_factory_(this) {
  DCHECK(rpc_controller);
  rpc_controller_.Bind(std::move(rpc_controller));
  DCHECK(rpc_controller_);
  rpc_controller_.set_disconnect_handler(base::BindOnce(
      &BraveWalletProviderImpl::OnConnectionError, weak_factory_.GetWeakPtr()));
}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {}

bool BraveWalletProviderImpl::OnAddEthereumChainRequest(
    const std::string& json_payload, RequestCallback callback) {
  AddEthereumChainParameter result;
  if (!ParseAddEthereumChainParameter(json_payload, &result))
    return false;
  if (!delegate_)
    return false;
  DLOG(INFO) << "json_payload:" << json_payload;
  delegate_->RequestUserApproval(kAddEthereumChainMethod, json_payload,
      base::BindOnce(&BraveWalletProviderImpl::OnChainAddedResult,
                    weak_factory_.GetWeakPtr(), std::move(callback)));
  return true;
}

void BraveWalletProviderImpl::Request(const std::string& json_payload,
                                      bool auto_retry_on_network_change,
                                      RequestCallback callback) {
  std::string method = brave_wallet::ParseRequestMethodName(json_payload);
  if (method == kAddEthereumChainMethod &&
      OnAddEthereumChainRequest(json_payload, std::move(callback))) {
    return;
  }
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

void BraveWalletProviderImpl::OnChainAddedResult(
    RequestCallback callback,
                const std::vector<std::string>& accounts) {
  base::flat_map<std::string, std::string> headers;
  std::move(callback).Run(200, "{'lalla': 1}", headers);
}

}  // namespace brave_wallet
