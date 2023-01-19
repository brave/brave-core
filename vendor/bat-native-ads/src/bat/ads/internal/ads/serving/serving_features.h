/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_SERVING_FEATURES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_SERVING_FEATURES_H_

#include "base/feature_list.h"  // IWYU pragma: keep

namespace base {
class TimeDelta;
}  // namespace base

namespace ads::features {

BASE_DECLARE_FEATURE(kServing);

bool IsServingEnabled();

int GetServingVersion();

int GetDefaultNotificationAdsPerHour();
int GetMaximumNotificationAdsPerDay();

int GetMaximumInlineContentAdsPerHour();
int GetMaximumInlineContentAdsPerDay();

int GetMaximumNewTabPageAdsPerHour();
int GetMaximumNewTabPageAdsPerDay();
base::TimeDelta GetNewTabPageAdsMinimumWaitTime();

int GetMaximumPromotedContentAdsPerHour();
int GetMaximumPromotedContentAdsPerDay();

int GetMaximumSearchResultAdsPerHour();
int GetMaximumSearchResultAdsPerDay();

int GetBrowsingHistoryMaxCount();
int GetBrowsingHistoryDaysAgo();

}  // namespace ads::features

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_SERVING_SERVING_FEATURES_H_
