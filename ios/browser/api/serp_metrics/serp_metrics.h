/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface SerpMetrics : NSObject

/// Number of Brave Search SERP visits recorded yesterday.
@property(readonly) NSInteger braveSearchCountForYesterday;
/// Number of Google Search SERP visits recorded yesterday.
@property(readonly) NSInteger googleSearchCountForYesterday;
/// Number of non-Brave, non-Google SERP visits recorded yesterday.
@property(readonly) NSInteger otherSearchCountForYesterday;
/// Number of SERP visits recorded between the last successful ping date and
/// the start of yesterday.
@property(readonly) NSInteger searchCountForStalePeriod;

/// Clears all stored SERP metrics history.
- (void)clearHistory;

- (instancetype)init NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_H_
