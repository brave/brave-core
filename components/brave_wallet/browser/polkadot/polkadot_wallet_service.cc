/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

#include "mojo/public/cpp/bindings/callback_helpers.h"

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
  auto weak = weak_ptr_factory_.GetWeakPtr();

  auto cb = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), "PolkadotWalletService unavailable.");

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PolkadotWalletService::StartGetNetworkName,
                                std::move(weak), std::move(cb)));
}

void PolkadotWalletService::StartGetNetworkName(
    GetNetworkNameCallback callback) {
  auto weak = weak_ptr_factory_.GetWeakPtr();
  polkadot_substrate_rpc_.GetChainName(base::BindOnce(
      &PolkadotWalletService::OnGetnetworkName, weak, std::move(callback)));
}

void PolkadotWalletService::OnGetnetworkName(GetNetworkNameCallback callback,
                                             std::string const& str) {
  std::move(callback).Run(str);
}

}  // namespace brave_wallet
