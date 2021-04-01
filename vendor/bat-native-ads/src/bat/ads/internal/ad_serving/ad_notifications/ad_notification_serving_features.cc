/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_notifications/ad_notification_serving_features.h"
#include "base/metrics/field_trial_params.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace features {

namespace {

const char kFeatureName[] = "AdNotificationServing";

const char kFieldTrialParameterBrowsingHistoryMaxCount[] =
    "browsing_history_max_count";

const int kDefaultBrowsingHistoryMaxCount = 5000;

const char kFieldTrialParameterBrowsingHistoryDaysAgo[] =
    "browsing_history_days_ago";
const int kDefaultBrowsingHistoryDaysAgo = 180;

}  // namespace

const base::Feature kAdNotificationServing{kFeatureName,
                                           base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAdNotificationServingEnabled() {
  return base::FeatureList::IsEnabled(kAdNotificationServing);
}

int GetBrowsingHistoryMaxCount() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotificationServing, kFieldTrialParameterBrowsingHistoryMaxCount,
      kDefaultBrowsingHistoryMaxCount);
}

int GetBrowsingHistoryDaysAgo() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdNotificationServing, kFieldTrialParameterBrowsingHistoryDaysAgo,
      kDefaultBrowsingHistoryDaysAgo);
}

}  // namespace features
}  // namespace ads
