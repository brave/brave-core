// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_FACTORY_BRIDGE_H_
#define BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_FACTORY_BRIDGE_H_

#import <Foundation/Foundation.h>

@protocol MostVisitedSitesBridge;
@protocol ProfileBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(MostVisitedSitesFactory)
@interface MostVisitedSitesFactoryBridge : NSObject

- (instancetype)init NS_UNAVAILABLE;

/// Creates a `MostVisitedSites` for `profile`
/// or `nil` when `profile` is off the record.
+ (id<MostVisitedSitesBridge>)mostVisitedSitesForProfile:
    (id<ProfileBridge>)profile
    NS_SWIFT_NAME(mostVisitedSites(for:)) NS_SWIFT_UI_ACTOR;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_MOST_VISITED_SITES_MOST_VISITED_SITES_FACTORY_BRIDGE_H_
