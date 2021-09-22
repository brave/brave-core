/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/factory/eth_tx_controller_factory_helper.h"

#include <utility>

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "components/user_prefs/user_prefs.h"

namespace brave_wallet {

std::unique_ptr<EthTxController> BuildEthTxController(
    EthJsonRpcController* rpc_controller,
    KeyringController* keyring_controller,
    PrefService* prefs) {
  auto tx_state_manager =
      std::make_unique<EthTxStateManager>(prefs, rpc_controller->MakeRemote());
  auto eth_nonce_tracker =
      std::make_unique<EthNonceTracker>(tx_state_manager.get(), rpc_controller);
  auto eth_pending_tx_tracker = std::make_unique<EthPendingTxTracker>(
      tx_state_manager.get(), rpc_controller, eth_nonce_tracker.get());
  return std::make_unique<EthTxController>(
      rpc_controller, keyring_controller, std::move(tx_state_manager),
      std::move(eth_nonce_tracker), std::move(eth_pending_tx_tracker), prefs);
}

}  // namespace brave_wallet
