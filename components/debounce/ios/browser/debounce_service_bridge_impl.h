// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_DEBOUNCE_IOS_BROWSER_DEBOUNCE_SERVICE_BRIDGE_IMPL_H_
#define BRAVE_COMPONENTS_DEBOUNCE_IOS_BROWSER_DEBOUNCE_SERVICE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "brave/components/debounce/ios/browser/debounce_service_bridge.h"

namespace debounce {
class DebounceService;
}

NS_ASSUME_NONNULL_BEGIN

@interface DebounceServiceBridgeImpl : NSObject <DebounceServiceBridge>
@property(readonly) raw_ptr<debounce::DebounceService> debounceService;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDebounceService:
    (raw_ptr<debounce::DebounceService>)debounceService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_DEBOUNCE_IOS_BROWSER_DEBOUNCE_SERVICE_BRIDGE_IMPL_H_
