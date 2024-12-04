/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_
#define BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NSInteger P3AMetricLogType NS_TYPED_ENUM;
OBJC_EXPORT P3AMetricLogType const P3AMetricLogTypeSlow;
OBJC_EXPORT P3AMetricLogType const P3AMetricLogTypeTypical;
OBJC_EXPORT P3AMetricLogType const P3AMetricLogTypeExpress;

OBJC_EXPORT NSString* const P3ACreativeMetricPrefix;

OBJC_EXPORT
@interface P3ACallbackRegistration : NSObject
@end

OBJC_EXPORT
@interface BraveP3AUtils : NSObject
@property(nonatomic) bool isP3AEnabled;
@property(nonatomic) bool isNoticeAcknowledged;
- (nullable P3ACallbackRegistration*)registerRotationCallback:
    (void (^)(P3AMetricLogType logType, BOOL isConstellation))callback;
- (nullable P3ACallbackRegistration*)registerMetricCycledCallback:
    (void (^)(NSString* histogramName, BOOL isConstellation))callback;
- (void)registerDynamicMetric:(NSString*)histogramName
                      logType:(P3AMetricLogType)logType;
- (void)registerDynamicMetric:(NSString*)histogramName
                      logType:(P3AMetricLogType)logType
              mainThreadBound:(BOOL)mainThreadBound;
- (void)removeDynamicMetric:(NSString*)histogramName;
- (void)updateMetricValueForSingleFormat:(NSString*)histogramName
                                  bucket:(size_t)bucket
                         isConstellation:(BOOL)isConstellation
    NS_SWIFT_NAME(updateMetricValueForSingleFormat(name:bucket:isConstellation:));
- (instancetype)init NS_UNAVAILABLE;
@end

/// For numeric measurements where you want exact integer values up to
/// |exclusive_max|. |exclusive_max| itself is included in the overflow bucket.
/// Therefore, if you want an accurate measure up to kMax, then |exclusive_max|
/// should be set to kMax + 1.
///
/// |exclusive_max| should be 101 or less. If you need to capture a larger
/// range, we recommend the use of the COUNT histograms below.
OBJC_EXPORT void UmaHistogramExactLinear(NSString* name,
                                         NSInteger sample,
                                         NSInteger exclusiveMax);
/// For adding boolean sample to histogram.
OBJC_EXPORT void UmaHistogramBoolean(NSString* name, bool sample);
/// For adding histogram sample denoting a percentage.
/// Percents are integers between 1 and 100, inclusively.
OBJC_EXPORT void UmaHistogramPercentage(NSString* name, NSInteger percent);
/// For adding counts histogram.
OBJC_EXPORT void UmaHistogramCustomCounts(NSString* name,
                                          NSInteger sample,
                                          NSInteger min,
                                          NSInteger exclusiveMax,
                                          size_t buckets);
/// For histograms storing times. It uses milliseconds granularity.
OBJC_EXPORT void UmaHistogramCustomTimes(NSString* name,
                                         NSTimeInterval sample,
                                         NSTimeInterval min,
                                         NSTimeInterval max,
                                         size_t buckets);
/// For short timings from 1 ms up to 10 seconds (50 buckets).
OBJC_EXPORT void UmaHistogramTimes(NSString* name, NSTimeInterval sample);
/// For medium timings up to 3 minutes (50 buckets).
OBJC_EXPORT void UmaHistogramMediumTimes(NSString* name, NSTimeInterval sample);
/// For time intervals up to 1 hr (50 buckets).
OBJC_EXPORT void UmaHistogramLongTimes(NSString* name, NSTimeInterval sample);
/// For recording memory related histograms.
/// Used to measure common KB-granularity memory stats. Range is up to 500M.
OBJC_EXPORT void UmaHistogramMemoryKB(NSString* name, NSInteger sample);
/// Used to measure common MB-granularity memory stats. Range is up to ~1G.
OBJC_EXPORT void UmaHistogramMemoryMB(NSString* name, NSInteger sample);
/// Used to measure common MB-granularity memory stats. Range is up to ~64G.
OBJC_EXPORT void UmaHistogramMemoryLargeMB(NSString* name, NSInteger sample);

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_P3A_BRAVE_P3A_UTILS_H_
