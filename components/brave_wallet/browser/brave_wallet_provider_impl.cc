/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_provider_impl.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"

namespace brave_wallet {

BraveWalletProviderImpl::BraveWalletProviderImpl(
    base::WeakPtr<BraveWalletService> wallet_service)
    : wallet_service_(wallet_service), weak_factory_(this) {}

BraveWalletProviderImpl::~BraveWalletProviderImpl() {
  if (!wallet_service_)
    return;

  auto* rpc_controller = wallet_service_->rpc_controller();
  rpc_controller->RemoveObserver(this);
}

void BraveWalletProviderImpl::Request(const std::string& json_payload,
                                      RequestCallback callback) {
  if (!wallet_service_)
    return;

  auto* rpc_controller = wallet_service_->rpc_controller();
  rpc_controller->Request(
      json_payload,
      base::BindOnce(&BraveWalletProviderImpl::OnResponse,
                     weak_factory_.GetWeakPtr(), std::move(callback)),
      true);
}

void BraveWalletProviderImpl::OnResponse(
    RequestCallback callback,
    const int http_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers) {
  // Do we need to pass headers map to a renderer? We would need to convert
  // it to base::flat_map in that case
  std::move(callback).Run(http_code, response);
}

void BraveWalletProviderImpl::GetChainId(GetChainIdCallback callback) {
  if (!wallet_service_)
    return;

  auto* rpc_controller = wallet_service_->rpc_controller();
  std::move(callback).Run(EthJsonRpcController::GetChainIDFromNetwork(
      rpc_controller->GetNetwork()));
}

void BraveWalletProviderImpl::Init(
    ::mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
    if (!wallet_service_)
      return;

    auto* rpc_controller = wallet_service_->rpc_controller();
    rpc_controller->AddObserver(this);
  }
}

void BraveWalletProviderImpl::ChainChangedEvent(const std::string& chain_id) {
  if (!events_listener_.is_bound())
    return;

  events_listener_->ChainChangedEvent(chain_id);
}

}  // namespace brave_wallet
