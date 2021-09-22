/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_FACTORY_ETH_TX_CONTROLLER_FACTORY_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_FACTORY_ETH_TX_CONTROLLER_FACTORY_HELPER_H_

#include <memory>

class PrefService;

namespace brave_wallet {

class EthJsonRpcController;
class EthTxController;
class KeyringController;

std::unique_ptr<EthTxController> BuildEthTxController(
    EthJsonRpcController* rpc_controller,
    KeyringController* keyring_controller,
    PrefService* prefs);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_FACTORY_ETH_TX_CONTROLLER_FACTORY_HELPER_H_
