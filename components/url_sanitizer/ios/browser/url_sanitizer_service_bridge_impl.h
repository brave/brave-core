// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_URL_SANITIZER_IOS_BROWSER_URL_SANITIZER_SERVICE_BRIDGE_IMPL_H_
#define BRAVE_COMPONENTS_URL_SANITIZER_IOS_BROWSER_URL_SANITIZER_SERVICE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "brave/components/url_sanitizer/ios/browser/url_sanitizer_service_bridge.h"

namespace brave {
class URLSanitizerService;
}

NS_ASSUME_NONNULL_BEGIN

@interface URLSanitizerServiceBridgeImpl : NSObject <URLSanitizerServiceBridge>
@property(readonly) raw_ptr<brave::URLSanitizerService> urlSanitizerService;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithURLSanitizerService:
    (raw_ptr<brave::URLSanitizerService>)urlSanitizerService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_URL_SANITIZER_IOS_BROWSER_URL_SANITIZER_SERVICE_BRIDGE_IMPL_H_
