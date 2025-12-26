/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_STATS_BRAVE_STATS_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_STATS_BRAVE_STATS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT NSString* const kBraveStatsAPIKey;
/// The endpoint we want to submit webcompat reports to
OBJC_EXPORT NSString* const kWebcompatReportEndpoint;

OBJC_EXPORT
@interface BraveStats : NSObject
@property(readonly) BOOL isStatsReportingManaged;
@property(nonatomic, getter=isStatsReportingEnabled)
    BOOL statsReportingEnabled NS_SWIFT_NAME(isStatsReportingEnabled);
@property(readonly, getter=isNotificationAdsEnabled)
    BOOL notificationAdsEnabled;
- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_STATS_BRAVE_STATS_H_
