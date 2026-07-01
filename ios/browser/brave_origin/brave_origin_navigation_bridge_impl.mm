// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_origin/brave_origin_navigation_bridge_impl.h"

#include "brave/ios/browser/skus/skus_service_factory.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

namespace brave_origin {

BraveOriginDelegateIOS::BraveOriginDelegateIOS(
    SkusServiceGetter skus_service_getter)
    : skus_service_getter_(std::move(skus_service_getter)) {}

BraveOriginDelegateIOS::~BraveOriginDelegateIOS() = default;

void BraveOriginDelegateIOS::OpenOriginSettings() {
  [BraveOriginNavigationBridge onOpenOriginSettings];
}

mojo::PendingRemote<skus::mojom::SkusService>
BraveOriginDelegateIOS::GetSkusService() {
  return skus_service_getter_.Run();
}

}  // namespace brave_origin

static void (^_openOriginSettings)() = nil;

@implementation BraveOriginNavigationBridge

+ (void (^)())openOriginSettings {
  return _openOriginSettings;
}

+ (void)setOpenOriginSettings:(void (^)())openOriginSettings {
  _openOriginSettings = [openOriginSettings copy];
}

+ (void)onOpenOriginSettings {
  if (self.openOriginSettings) {
    self.openOriginSettings();
  }
}

@end
