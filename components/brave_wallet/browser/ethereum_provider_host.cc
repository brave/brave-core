/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_provider_host.h"

namespace brave_wallet {

EthereumProviderHost::EthereumProviderHost() = default;
EthereumProviderHost::~EthereumProviderHost() = default;

mojo::PendingReceiver<mojom::EthereumProvider>
EthereumProviderHost::BindRemote() {
  return ethereum_provider_service_.BindNewPipeAndPassReceiver();
}

void EthereumProviderHost::Init(
    mojo::PendingRemote<mojom::EventsListener> events_listener) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->Init(std::move(events_listener));
  }
}

void EthereumProviderHost::Request(base::Value input,
                                   RequestCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->Request(std::move(input), std::move(callback));
  }
}

void EthereumProviderHost::Enable(EnableCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->Enable(std::move(callback));
  }
}

void EthereumProviderHost::Send(const std::string& method,
                                base::Value params,
                                SendCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->Send(method, std::move(params),
                                     std::move(callback));
  }
}

void EthereumProviderHost::SendAsync(base::Value input,
                                     SendAsyncCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->SendAsync(std::move(input),
                                          std::move(callback));
  }
}

void EthereumProviderHost::GetChainId(GetChainIdCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->GetChainId(std::move(callback));
  }
}

void EthereumProviderHost::IsLocked(IsLockedCallback callback) {
  if (ethereum_provider_service_.is_bound()) {
    ethereum_provider_service_->IsLocked(std::move(callback));
  }
}

}  // namespace brave_wallet
