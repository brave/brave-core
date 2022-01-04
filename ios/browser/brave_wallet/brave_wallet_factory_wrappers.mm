/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_factory_wrappers.h"

#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/eth_tx_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveWalletAssetRatioServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::AssetRatioServiceFactory::GetForBrowserState(
          browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletAssetRatioServiceImpl alloc]
      initWithAssetRatioService:controller];
}
@end

@implementation BraveWalletEthJsonRpcControllerFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::EthJsonRpcControllerFactory::GetForBrowserState(
          browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletEthJsonRpcControllerImpl alloc]
      initWithEthJsonRpcController:controller];
}
@end

@implementation BraveWalletEthTxServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::EthTxServiceFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletEthTxServiceImpl alloc]
      initWithEthTxService:controller];
}
@end

@implementation BraveWalletKeyringControllerFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::KeyringControllerFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletKeyringControllerImpl alloc]
      initWithKeyringController:controller];
}
@end

@implementation BraveWalletServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::BraveWalletServiceFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletBraveWalletServiceImpl alloc]
      initWithBraveWalletService:controller];
}
@end

@implementation BraveWalletSwapServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::SwapServiceFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return
      [[BraveWalletSwapServiceImpl alloc] initWithSwapService:controller];
}
@end
