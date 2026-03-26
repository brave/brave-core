/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/serp_metrics/serp_metrics.h"

#include "base/check.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics.h"
#include "brave/ios/browser/api/serp_metrics/serp_metrics+private.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

@implementation SerpMetrics {
  raw_ptr<serp_metrics::SerpMetrics> _serp_metrics;
}

- (instancetype)initWithProfile:(ProfileIOS*)profile {
  if ((self = [super init])) {
    _serp_metrics =
        serp_metrics::SerpMetricsServiceFactoryIOS::GetForProfile(profile);
  }
  return self;
}

- (NSInteger)braveSearchCountForYesterday {
  CHECK(_serp_metrics);
  return static_cast<NSInteger>(_serp_metrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kBrave));
}

- (NSInteger)googleSearchCountForYesterday {
  CHECK(_serp_metrics);
  return static_cast<NSInteger>(_serp_metrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kGoogle));
}

- (NSInteger)otherSearchCountForYesterday {
  CHECK(_serp_metrics);
  return static_cast<NSInteger>(_serp_metrics->GetSearchCountForYesterday(
      serp_metrics::SerpMetricType::kOther));
}

- (NSInteger)searchCountForStalePeriod {
  CHECK(_serp_metrics);
  return static_cast<NSInteger>(_serp_metrics->GetSearchCountForStalePeriod());
}

- (void)clearHistory {
  if (_serp_metrics) {
    _serp_metrics->ClearHistory();
  }
}

@end
