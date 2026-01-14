// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_origin/brave_origin_service_factory_bridge.h"

#include "brave/ios/browser/brave_origin/brave_origin_service_bridge_impl.h"
#include "brave/ios/browser/brave_origin/brave_origin_service_factory.h"

@implementation BraveOriginServiceFactoryBridge

+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  auto* service =
      brave_origin::BraveOriginServiceFactory::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  return
      [[BraveOriginServiceBridgeImpl alloc] initWithBraveOriginService:service];
}

@end
