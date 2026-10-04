/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_BRIDGE_H_
#define BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_BRIDGE_H_

#import <Foundation/Foundation.h>

#include "keyed_service_factory_wrapper.h"  // NOLINT

@protocol SerpMetricsBridge;

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
NS_SWIFT_NAME(SerpMetricsServiceFactory)
@interface SerpMetricsServiceFactoryBridge
    : KeyedServiceFactoryWrapper <id <SerpMetricsBridge>>
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_SERP_METRICS_SERP_METRICS_SERVICE_FACTORY_BRIDGE_H_
