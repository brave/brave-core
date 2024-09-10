/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/skus/skus_sdk_factory_wrappers.h"

#include "brave/ios/browser/api/skus/skus_sdk.mojom.objc+private.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#include "brave/ios/browser/skus/skus_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation SkusSkusServiceFactory
+ (nullable id)serviceForBrowserState:(ChromeBrowserState*)browserState {
  auto service = skus::SkusServiceFactory::GetForBrowserState(browserState);
  if (!service) {
    return nil;
  }
  return
      [[SkusSkusServiceMojoImpl alloc] initWithSkusService:std::move(service)];
}
@end
