// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_HTTPS_UPGRADES_HTTPS_UPGRADE_SERVICE_H_
#define BRAVE_IOS_BROWSER_API_HTTPS_UPGRADES_HTTPS_UPGRADE_SERVICE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(HttpsUpgradeService)
@interface BraveHttpsUpgradeService : NSObject
- (bool)isHttpAllowedForHost:(NSString*)host;
- (void)allowHttpForHost:(NSString*)host;
- (void)clearAllowlistFromStartDate:(NSDate*)startDate endDate:(NSDate*)endDate;
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_HTTPS_UPGRADES_HTTPS_UPGRADE_SERVICE_H_
