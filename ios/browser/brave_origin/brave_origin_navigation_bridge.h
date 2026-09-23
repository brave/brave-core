// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_H_
#define BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// A navigation handler for Brave Origin's service delegate.
// Currently only opens the Origin settings page when a purchase is detected.
NS_SWIFT_NAME(BraveOriginNavigation)
OBJC_EXPORT
@interface BraveOriginNavigationBridge : NSObject
@property(nonatomic, class, copy) void (^openOriginSettings)();
- (instancetype)init NS_UNAVAILABLE;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_BRAVE_ORIGIN_BRAVE_ORIGIN_NAVIGATION_BRIDGE_H_
