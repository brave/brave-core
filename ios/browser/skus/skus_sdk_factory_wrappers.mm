/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_sdk_factory_wrappers.h"

#include "brave/ios/browser/api/skus/skus_sdk.mojom.objc+private.h"
#include "brave/ios/browser/skus/skus_sdk_service_factory.h"
#include "brave/ios/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/eth_tx_controller_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/ios/browser/brave_wallet/swap_controller_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SkusSkusSdkFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      skus::SkusSdkServiceFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return [[SkusSkusSdkImpl alloc] initWithSkusSdk:controller];
}
@end