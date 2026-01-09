// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "brave/ios/browser/brave_origin/brave_origin_service_bridge.h"

namespace brave_origin {
class BraveOriginService;
}

@interface BraveOriginServiceBridgeImpl : NSObject <BraveOriginServiceBridge>
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithBraveOriginService:
    (brave_origin::BraveOriginService*)service NS_DESIGNATED_INITIALIZER;
@end

#endif  // BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_SERVICE_BRIDGE_IMPL_H_
