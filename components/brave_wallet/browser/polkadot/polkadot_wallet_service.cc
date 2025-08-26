/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"

namespace brave_wallet {

PolkadotWalletService::PolkadotWalletService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : polkadot_rpc_(url_loader_factory) {
  polkadot_rpc_.GetSystemChain();
}

PolkadotWalletService::~PolkadotWalletService() = default;
}  // namespace brave_wallet
