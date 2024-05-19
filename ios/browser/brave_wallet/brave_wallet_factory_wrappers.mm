/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_factory_wrappers.h"

#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_service_factory.h"
#include "brave/ios/browser/brave_wallet/bitcoin_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_ipfs_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/json_rpc_service_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_service_factory.h"
#include "brave/ios/browser/brave_wallet/meld_integration_service_factory.h"
#include "brave/ios/browser/brave_wallet/swap_service_factory.h"
#include "brave/ios/browser/brave_wallet/tx_service_factory.h"
#include "brave/ios/browser/brave_wallet/zcash_wallet_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

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

@implementation BraveWalletBitcoinWalletServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service = brave_wallet::BitcoinWalletServiceFactory::GetForBrowserState(
      browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletBitcoinWalletServiceMojoImpl alloc]
      initWithBitcoinWalletService:std::move(service)];
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

@implementation BraveWalletTxServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::TxServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletTxServiceMojoImpl alloc]
      initWithTxService:std::move(service)];
}
@end

@implementation BraveWalletEthTxManagerProxyFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto proxy =
      brave_wallet::TxServiceFactory::GetEthTxManagerProxyForBrowserState(
          browserState);
  if (!proxy) {
    return nil;
  }
  return [[BraveWalletEthTxManagerProxyMojoImpl alloc]
      initWithEthTxManagerProxy:std::move(proxy)];
}
@end

@implementation BraveWalletSolanaTxManagerProxyFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto proxy =
      brave_wallet::TxServiceFactory::GetSolanaTxManagerProxyForBrowserState(
          browserState);
  if (!proxy) {
    return nil;
  }
  return [[BraveWalletSolanaTxManagerProxyMojoImpl alloc]
      initWithSolanaTxManagerProxy:std::move(proxy)];
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

@implementation BraveWalletMeldIntegrationServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::MeldIntegrationServiceFactory::GetForBrowserState(
          browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletMeldIntegrationServiceMojoImpl alloc]
      initWithMeldIntegrationService:std::move(service)];
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

@implementation BraveWalletIpfsServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::BraveWalletIpfsServiceFactory::GetForBrowserState(
          browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletIpfsServiceMojoImpl alloc]
      initWithIpfsService:std::move(service)];
}
@end

@implementation BraveWalletZCashWalletServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service =
      brave_wallet::ZCashWalletServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return [[BraveWalletZCashWalletServiceMojoImpl alloc]
      initWithZCashWalletService:std::move(service)];
}
@end
