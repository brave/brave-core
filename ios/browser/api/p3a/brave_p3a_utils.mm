/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/p3a/brave_p3a_utils.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/ios/browser/api/p3a/brave_histograms_controller+private.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BraveP3AUtils {
  ChromeBrowserState* _browserState;
  PrefService* _localState;
}

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState
                          localState:(PrefService*)localState {
  if ((self = [super init])) {
    _browserState = mainBrowserState;
    _localState = localState;
  }
  return self;
}

- (bool)isP3AEnabled {
  return _localState->GetBoolean(p3a::kP3AEnabled);
}

- (void)setIsP3AEnabled:(bool)isP3AEnabled {
  _localState->SetBoolean(p3a::kP3AEnabled, isP3AEnabled);
  _localState->CommitPendingWrite();
}

- (bool)isNoticeAcknowledged {
  return _localState->GetBoolean(p3a::kP3ANoticeAcknowledged);
}

- (void)setIsNoticeAcknowledged:(bool)isNoticeAcknowledged {
  _localState->SetBoolean(p3a::kP3ANoticeAcknowledged, isNoticeAcknowledged);
  _localState->CommitPendingWrite();
}

- (BraveHistogramsController*)histogramsController {
  return [[BraveHistogramsController alloc] initWithBrowserState:_browserState];
}

void UmaHistogramExactLinear(NSString* name,
                             NSInteger sample,
                             NSInteger exclusive_max) {
  base::UmaHistogramExactLinear(base::SysNSStringToUTF8(name), sample,
                                exclusive_max);
}

void UmaHistogramBoolean(NSString* name, bool sample) {
  base::UmaHistogramBoolean(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramPercentage(NSString* name, NSInteger percent) {
  base::UmaHistogramPercentage(base::SysNSStringToUTF8(name), percent);
}

void UmaHistogramCustomCounts(NSString* name,
                              NSInteger sample,
                              NSInteger min,
                              NSInteger exclusive_max,
                              size_t buckets) {
  base::UmaHistogramCustomCounts(base::SysNSStringToUTF8(name), sample, min,
                                 exclusive_max, buckets);
}

void UmaHistogramCustomTimes(NSString* name,
                             NSTimeInterval sample,
                             NSTimeInterval min,
                             NSTimeInterval max,
                             size_t buckets) {
  base::UmaHistogramCustomTimes(base::SysNSStringToUTF8(name),
                                base::Seconds(sample), base::Seconds(min),
                                base::Seconds(max), buckets);
}

void UmaHistogramTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramTimes(base::SysNSStringToUTF8(name), base::Seconds(sample));
}

void UmaHistogramMediumTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramMediumTimes(base::SysNSStringToUTF8(name),
                                base::Seconds(sample));
}

void UmaHistogramLongTimes(NSString* name, NSTimeInterval sample) {
  base::UmaHistogramLongTimes(base::SysNSStringToUTF8(name),
                              base::Seconds(sample));
}

void UmaHistogramMemoryKB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryKB(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramMemoryMB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryMB(base::SysNSStringToUTF8(name), sample);
}

void UmaHistogramMemoryLargeMB(NSString* name, NSInteger sample) {
  base::UmaHistogramMemoryLargeMB(base::SysNSStringToUTF8(name), sample);
}

@end
