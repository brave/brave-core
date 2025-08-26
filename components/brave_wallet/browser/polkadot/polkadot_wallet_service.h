/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_

#include "brave/components/brave_wallet/browser/polkadot/polkadot_rpc.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class PolkadotWalletService {
 public:
  explicit PolkadotWalletService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  ~PolkadotWalletService();

 private:
  PolkadotRpc polkadot_rpc_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_WALLET_SERVICE_H_
