/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_shields/brave_shields_utils_service_factory_wrapper.h"

#include "brave/ios/browser/api/brave_shields/brave_shields_utils_service_factory.h"
#include "brave/ios/browser/keyed_service/keyed_service_factory_wrapper+private.h"
#import "brave_shields_panel.mojom.objc.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation BraveShieldsBraveShieldsUtilsService
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto handler =
      brave_shields::BraveShieldsBraveShieldsUtilsService::GetHandlerForContext(
          profile);
  if (!handler) {
    return nil;
  }
  return [[BraveShieldsBraveShieldsUtilsServiceMojoImpl alloc]
      initWithBraveShieldsUtilsService:std::move(handler)];
}
@end
