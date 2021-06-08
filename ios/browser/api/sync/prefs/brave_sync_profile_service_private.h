/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_PREFS_BRAVE_SYNC_PROFILE_SERVICE_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_SYNC_PREFS_BRAVE_SYNC_PROFILE_SERVICE_PRIVATE_H_

#import <Foundation/Foundation.h>

#import "brave/ios/browser/api/sync/prefs/brave_sync_profile_service.h"

NS_ASSUME_NONNULL_BEGIN

class ChromeBrowserState;

OBJC_EXPORT
@interface BraveSyncProfileService (Private)  // NOLINT
- (instancetype)initWithBrowserState:(ChromeBrowserState*)state;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_PREFS_BRAVE_SYNC_PROFILE_SERVICE_PRIVATE_H_
