// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/https_upgrades/https_upgrade_service_factory.h"

#include "brave/ios/browser/api/https_upgrades/https_upgrade_service+private.h"
#include "ios/chrome/browser/https_upgrades/model/https_upgrade_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/components/security_interstitials/https_only_mode/https_upgrade_service.h"

@implementation BraveHttpsUpgradeServiceFactory
+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* service = HttpsUpgradeServiceFactory::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  return [[BraveHttpsUpgradeService alloc] initWithHttpsUpgradeService:service];
}
@end
