/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/factory/eth_tx_service_factory_helper.h"

#include <utility>

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_tx_service.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

std::unique_ptr<EthTxService> BuildEthTxService(
    JsonRpcService* json_rpc_service,
    KeyringService* keyring_service,
    AssetRatioService* asset_ratio_service,
    PrefService* prefs) {
  auto tx_state_manager =
      std::make_unique<EthTxStateManager>(prefs, json_rpc_service);
  auto eth_nonce_tracker = std::make_unique<EthNonceTracker>(
      tx_state_manager.get(), json_rpc_service);
  auto eth_pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
      tx_state_manager.get(), json_rpc_service, eth_nonce_tracker.get());
  return std::make_unique<EthTxService>(
      json_rpc_service, keyring_service, asset_ratio_service,
      std::move(tx_state_manager), std::move(eth_nonce_tracker),
      std::move(eth_pending_tx_tracker), prefs);
}

}  // namespace brave_wallet
