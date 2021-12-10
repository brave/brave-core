/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_sdk_factory_wrappers.h"

#include "brave/ios/browser/api/skus/skus_sdk.mojom.objc+private.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/skus/sdk_controller_factory.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SkusSdkControllerFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto* controller =
      skus::SdkControllerFactory::GetForBrowserState(browserState);
  if (!controller) {
    return nil;
  }
  return [[SkusSdkControllerImpl alloc] initWithSdkController:controller];
}
@end
