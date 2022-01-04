/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_reset.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/brave_wallet/eth_tx_service_factory.h"
#include "brave/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/browser/brave_wallet/keyring_service_factory.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/eth_tx_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"

namespace brave_wallet {

void ResetTransactionInfo(content::BrowserContext* context) {
  auto* eth_tx_service =
      brave_wallet::EthTxServiceFactory::GetControllerForContext(context);
  eth_tx_service->Reset();
}

void ResetWallet(content::BrowserContext* context) {
  ResetTransactionInfo(context);
  auto* json_rpc_service =
      brave_wallet::JsonRpcServiceFactory::GetControllerForContext(context);
  json_rpc_service->Reset();

  auto* brave_wallet_service =
      brave_wallet::BraveWalletServiceFactory::GetServiceForContext(context);
  brave_wallet_service->Reset();

  auto* keyring_service =
      brave_wallet::KeyringServiceFactory::GetControllerForContext(context);
  keyring_service->Reset();
}

}  // namespace brave_wallet
