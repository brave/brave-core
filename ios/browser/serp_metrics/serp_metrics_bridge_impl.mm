/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_bridge_impl.h"

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"

@implementation SerpMetricsBridgeImpl {
  // `raw_ref` cannot be used for here because ObjC ivars are zero-initialized
  // before `init` runs, but `raw_ref` cannot represent a null or default value.
  raw_ptr<serp_metrics::SerpMetrics> _serpMetrics;  // Not owned.
}

- (instancetype)initWithSerpMetrics:(serp_metrics::SerpMetrics&)serpMetrics {
  if ((self = [super init])) {
    _serpMetrics = &serpMetrics;
  }
  return self;
}

- (NSInteger)braveSearchCountForYesterday {
  return static_cast<NSInteger>(_serpMetrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kBrave));
}

- (NSInteger)googleSearchCountForYesterday {
  return static_cast<NSInteger>(_serpMetrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kGoogle));
}

- (NSInteger)otherSearchCountForYesterday {
  return static_cast<NSInteger>(_serpMetrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kOther));
}

- (NSInteger)searchCountForStalePeriod {
  return static_cast<NSInteger>(_serpMetrics->GetSearchCountForStalePeriod());
}

- (void)clearHistory {
  _serpMetrics->ClearHistory();
}

@end
