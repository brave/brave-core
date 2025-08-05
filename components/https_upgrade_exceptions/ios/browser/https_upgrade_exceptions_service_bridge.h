// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_H_
#define BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

NS_SWIFT_NAME(HTTPSUpgradeExceptionsService)
@protocol HTTPSUpgradeExceptionsServiceBridge
- (BOOL)canUpgradeToHTTPSForURL:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_COMPONENTS_HTTPS_UPGRADE_EXCEPTIONS_IOS_BROWSER_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_BRIDGE_H_
