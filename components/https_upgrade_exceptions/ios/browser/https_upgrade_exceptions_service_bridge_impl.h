// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_IMPL_H_
#define BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_IMPL_H_

#import <Foundation/Foundation.h>

#include "base/memory/raw_ptr.h"
#include "brave/components/https_upgrade_exceptions/ios/browser/https_upgrade_exceptions_service_bridge.h"

namespace https_upgrade_exceptions {
class HttpsUpgradeExceptionsService;
}

NS_ASSUME_NONNULL_BEGIN

@interface HTTPSUpgradeExceptionsServiceBridgeImpl
    : NSObject <HTTPSUpgradeExceptionsServiceBridge>
@property(readonly)
    raw_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>
        httpsUpgradeExceptionsService;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithHTTPSUpgradeExceptionsService:
    (raw_ptr<https_upgrade_exceptions::HttpsUpgradeExceptionsService>)
        httpsUpgradeExceptionsService;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_IMPL_H_
