/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

BraveWalletService::BraveWalletService(content::BrowserContext* context)
    : context_(context) {
  rpc_controller_ = std::make_unique<brave_wallet::EthJsonRpcController>(
      context, brave_wallet::Network::kMainnet);
  keyring_controller_ = std::make_unique<brave_wallet::KeyringController>(
      user_prefs::UserPrefs::Get(context_));
}

BraveWalletService::~BraveWalletService() {}

brave_wallet::EthJsonRpcController* BraveWalletService::rpc_controller() const {
  return rpc_controller_.get();
}

brave_wallet::KeyringController* BraveWalletService::keyring_controller()
    const {
  return keyring_controller_.get();
}
