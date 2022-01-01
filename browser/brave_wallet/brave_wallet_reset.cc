/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_reset.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/browser/brave_wallet/rpc_controller_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"

namespace brave_wallet {

void ResetTransactionInfo(content::BrowserContext* context) {
  auto* eth_tx_controller =
      brave_wallet::EthTxControllerFactory::GetControllerForContext(context);
  eth_tx_controller->Reset();
}

void ResetWallet(content::BrowserContext* context) {
  ResetTransactionInfo(context);
  auto* rpc_controller =
      brave_wallet::RpcControllerFactory::GetControllerForContext(context);
  rpc_controller->Reset();

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(context);
  brave_wallet_service->Reset();

  auto* keyring_controller =
      brave_wallet::KeyringControllerFactory::GetControllerForContext(context);
  keyring_controller->Reset();
}

}  // namespace brave_wallet
