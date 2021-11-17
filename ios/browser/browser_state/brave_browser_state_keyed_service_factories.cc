/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"

#include "brave/ios/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/ios/browser/brave_wallet/swap_controller_factory.h"
#include "brave/ios/browser/skus/skus_sdk_service_factory.h"

namespace brave {

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  brave_wallet::AssetRatioControllerFactory::GetInstance();
  brave_wallet::BraveWalletServiceFactory::GetInstance();
  brave_wallet::EthJsonRpcControllerFactory::GetInstance();
  brave_wallet::EthTxControllerFactory::GetInstance();
  brave_wallet::KeyringControllerFactory::GetInstance();
  brave_wallet::SwapControllerFactory::GetInstance();
  skus::SkusSdkServiceFactory::GetInstance();
}

}  // namespace brave
