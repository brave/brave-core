/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/features/ad_serving/ad_serving_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace features {

namespace {

const char kFeatureName[] = "AdServing";

const char kFieldTrialParameterDefaultAdNotificationsPerHour[] =
    "default_ad_notifications_per_hour";
const int kDefaultDefaultAdNotificationsPerHour =
    kDefaultAdNotificationsPerHour;
const char kFieldTrialParameterMaximumAdNotificationsPerDay[] =
    "maximum_ad_notifications_per_day";
const int kDefaultMaximumAdNotificationsPerDay = 40;

const char kFieldTrialParameterMaximumNewTabPageAdsPerHour[] =
    "maximum_new_tab_page_ads_per_hour";
const int kDefaultMaximumNewTabPageAdsPerHour = 4;
const char kFieldTrialParameterMaximumNewTabPageAdsPerDay[] =
    "maximum_new_tab_page_ads_per_day";
const int kDefaultMaximumNewTabPageAdsPerDay = 20;

const char kFieldTrialParameterMaximumPromotedContentAdsPerHour[] =
    "maximum_promoted_content_ads_per_hour";
const int kDefaultMaximumPromotedContentAdsPerHour = 4;
const char kFieldTrialParameterMaximumPromotedContentAdsPerDay[] =
    "maximum_promoted_content_ads_per_day";
const int kDefaultMaximumPromotedContentAdsPerDay = 20;

}  // namespace

const base::Feature kAdServing{kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT};

bool IsAdServingEnabled() {
  return base::FeatureList::IsEnabled(kAdServing);
}

int GetDefaultAdNotificationsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterDefaultAdNotificationsPerHour,
      kDefaultDefaultAdNotificationsPerHour);
}

int GetMaximumAdNotificationsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumAdNotificationsPerDay,
      kDefaultMaximumAdNotificationsPerDay);
}

int GetMaximumNewTabPageAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumNewTabPageAdsPerHour,
      kDefaultMaximumNewTabPageAdsPerHour);
}

int GetMaximumNewTabPageAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumNewTabPageAdsPerDay,
      kDefaultMaximumNewTabPageAdsPerDay);
}

int GetMaximumPromotedContentAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumPromotedContentAdsPerHour,
      kDefaultMaximumPromotedContentAdsPerHour);
}

int GetMaximumPromotedContentAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumPromotedContentAdsPerDay,
      kDefaultMaximumPromotedContentAdsPerDay);
}

}  // namespace features
}  // namespace ads
