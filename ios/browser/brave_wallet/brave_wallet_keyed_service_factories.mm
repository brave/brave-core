/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_wallet/brave_wallet_keyed_service_factories.h"

#include "brave/ios/browser/api/brave_wallet/brave_wallet.mojom.objc+private.h"
#include "brave/ios/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/ios/components/keyed_service/keyed_service_factory_wrapper+private.h"
#include "components/keyed_service/ios/browser_state_dependency_manager.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation AssetRatioControllerFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      brave_wallet::AssetRatioControllerFactory::GetForBrowserState(
          browserState);
  if (!controller) {
    return nil;
  }
  return [[BraveWalletAssetRatioControllerImpl alloc]
      initWithAssetRatioController:controller];
}
@end
