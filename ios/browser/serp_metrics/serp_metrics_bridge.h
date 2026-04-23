/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Records and aggregates search engine usage counts.
NS_SWIFT_NAME(SerpMetrics)
@protocol SerpMetricsBridge
/// Brave Search engine usage counts recorded yesterday.
@property(readonly) NSInteger braveSearchCountForYesterday;
/// Google Search engine usage counts recorded yesterday.
@property(readonly) NSInteger googleSearchCountForYesterday;
/// Non-Brave, non-Google search engine usage counts recorded yesterday.
@property(readonly) NSInteger otherSearchCountForYesterday;
/// Search engine usage counts recorded for the stale period.
@property(readonly) NSInteger searchCountForStalePeriod;
/// Clears all stored search engine usage counts.
- (void)clearHistory;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_H_
