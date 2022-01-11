/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_factory_wrappers.h"

#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/eth_tx_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveWalletAssetRatioServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::AssetRatioServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletAssetRatioServiceMojoImpl alloc]
      initWithAssetRatioService:std::move(service)];
}
@end

@implementation BraveWalletJsonRpcServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::JsonRpcServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletJsonRpcServiceMojoImpl alloc]
      initWithJsonRpcService:std::move(service)];
}
@end

@implementation BraveWalletEthTxServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::EthTxServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletEthTxServiceMojoImpl alloc]
      initWithEthTxService:std::move(service)];
}
@end

@implementation BraveWalletKeyringServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::KeyringServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletKeyringServiceMojoImpl alloc]
      initWithKeyringService:std::move(service)];
}
@end

@implementation BraveWalletServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::BraveWalletServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletBraveWalletServiceMojoImpl alloc]
      initWithBraveWalletService:std::move(service)];
}
@end

@implementation BraveWalletSwapServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::SwapServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletSwapServiceMojoImpl alloc]
      initWithSwapService:std::move(service)];
}
@end
