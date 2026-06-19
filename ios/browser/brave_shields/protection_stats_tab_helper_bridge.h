// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface ProtectionStatsResource : NSObject
@property(readonly) NSString* resourceURL;
@property(readonly) NSString* resourceType;
- (instancetype)initWithURL:(NSString*)url
                       type:(NSString*)type NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
@end

@protocol ProtectionStatsTabHelperBridge
@required
// Called when a frame reports resources that may be blocked by Shields.
// `resources` is an array of dictionaries each containing a "resourceURL" and
// a "resourceType" key. `securityOrigin` is the security origin of the frame
// that reported the resources.
- (void)handleBlockedResources:(NSArray<ProtectionStatsResource*>*)resources
                securityOrigin:(NSURL*)securityOrigin;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_BRIDGE_H_
