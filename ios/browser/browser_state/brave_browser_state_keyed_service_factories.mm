/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"

#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/brave_wallet/tx_service_factory.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "brave/ios/browser/url_sanitizer/url_sanitizer_service_factory+private.h"

namespace brave {

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  brave_wallet::AssetRatioServiceFactory::GetInstance();
  brave_wallet::BraveWalletServiceFactory::GetInstance();
  brave_wallet::JsonRpcServiceFactory::GetInstance();
  brave_wallet::TxServiceFactory::GetInstance();
  brave_wallet::KeyringServiceFactory::GetInstance();
  brave_wallet::SwapServiceFactory::GetInstance();
  skus::SkusServiceFactory::GetInstance();
  brave::URLSanitizerServiceFactory::GetInstance();
}

}  // namespace brave
