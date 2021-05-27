/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

BraveWalletService::BraveWalletService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  rpc_controller_ = std::make_unique<brave_wallet::EthJsonRpcController>(
      brave_wallet::Network::kMainnet, url_loader_factory);
  keyring_controller_ =
      std::make_unique<brave_wallet::KeyringController>(prefs);
}

BraveWalletService::~BraveWalletService() {}

brave_wallet::EthJsonRpcController* BraveWalletService::rpc_controller() const {
  return rpc_controller_.get();
}

brave_wallet::KeyringController* BraveWalletService::keyring_controller()
    const {
  return keyring_controller_.get();
}
