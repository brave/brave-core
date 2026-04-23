/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_bridge.h"

#include "brave/components/serp_metrics/serp_metrics_service.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_bridge_impl.h"
#include "brave/ios/browser/serp_metrics/serp_metrics_service_factory_ios.h"

@implementation SerpMetricsServiceFactoryBridge

+ (nullable id)serviceForProfile:(ProfileIOS*)profile {
  serp_metrics::SerpMetricsService* service =
      serp_metrics::SerpMetricsServiceFactoryIOS::GetForProfile(profile);
  if (!service) {
    return nil;
  }
  serp_metrics::SerpMetrics* metrics = service->Get();
  if (!metrics) {
    return nil;
  }
  return [[SerpMetricsBridgeImpl alloc] initWithSerpMetrics:*metrics];
}

@end
