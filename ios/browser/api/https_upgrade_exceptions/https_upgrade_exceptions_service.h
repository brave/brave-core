// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface HTTPSUpgradeExceptionsService : NSObject
/// Tells us if the "HTTPS by default" feature is enabled
@property(readonly) bool isHttpsByDefaultFeatureEnabled;

- (instancetype)init;
/// This returns a new URL with HTTPS if the url can be upgraded to HTTPS
- (nullable NSURL*)upgradeToHTTPSForURL:(NSURL*)url;
/// Add an exception for the URL so it will no longer upgrade it to HTTPS
/// It will use the host of the URL to add the exception
- (void)addExceptionForURL:(NSURL*)url;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HTTPS_UPGRADE_EXCEPTIONS_HTTPS_UPGRADE_EXCEPTIONS_SERVICE_H_
