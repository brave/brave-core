/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_IMPL_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_IMPL_H_

#include "brave/ios/browser/serp_metrics/serp_metrics_bridge.h"

namespace serp_metrics {
class SerpMetrics;
}

OBJC_EXPORT
@interface SerpMetricsBridgeImpl : NSObject <SerpMetricsBridge>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithSerpMetrics:(serp_metrics::SerpMetrics&)serpMetrics
    NS_DESIGNATED_INITIALIZER;

@end

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_BRIDGE_IMPL_H_
