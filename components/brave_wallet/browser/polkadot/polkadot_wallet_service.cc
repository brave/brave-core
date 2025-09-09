/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

namespace brave_wallet {

PolkadotWalletService::PolkadotWalletService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : polkadot_substrate_rpc_(std::move(url_loader_factory)) {}

PolkadotWalletService::~PolkadotWalletService() = default;

void PolkadotWalletService::Bind(
    mojo::PendingReceiver<mojom::PolkadotWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void PolkadotWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void PolkadotWalletService::GetNetworkName(GetNetworkNameCallback callback) {
  polkadot_substrate_rpc_.GetChainName(std::move(callback));
}

}  // namespace brave_wallet
