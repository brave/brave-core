/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_PRIVATE_H_

#include "brave/ios/browser/api/serp_metrics/serp_metrics.h"

class ProfileIOS;

@interface SerpMetrics (Private)
- (instancetype)initWithProfile:(ProfileIOS*)profile;
@end

#endif  // BRAVE_IOS_BROWSER_API_SERP_METRICS_SERP_METRICS_PRIVATE_H_
