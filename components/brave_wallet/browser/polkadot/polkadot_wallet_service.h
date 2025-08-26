/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "brave/components/brave_wallet/browser/polkadot/polkadot_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class PolkadotWalletService : public mojom::PolkadotWalletService {
 public:
  explicit PolkadotWalletService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ~PolkadotWalletService() override;

  void Bind(mojo::PendingReceiver<mojom::PolkadotWalletService> receiver);
  void Reset();

  // mojom::CardanoWalletService:
  void GetNetworkName(GetNetworkNameCallback callback) override;

 private:
  mojo::ReceiverSet<mojom::PolkadotWalletService> receivers_;

  PolkadotRpc polkadot_rpc_;
  base::WeakPtrFactory<PolkadotWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
