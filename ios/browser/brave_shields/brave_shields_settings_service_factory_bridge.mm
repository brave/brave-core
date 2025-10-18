// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/brave_shields_settings_service_factory_bridge.h"

#include "base/apple/foundation_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_settings_service.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge.h"
#include "brave/components/brave_shields/ios/browser/brave_shields_settings_bridge_impl.h"
#include "brave/ios/browser/brave_shields/brave_shields_settings_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation BraveShieldsSettingsServiceFactoryBridge

+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* service =
      brave_shields::BraveShieldsSettingsServiceFactory::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  return [[BraveShieldsSettingsBridgeImpl alloc]
      initWithBraveShieldsSettings:service];
}

@end
