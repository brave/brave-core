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

const char kFieldTrialParameterMaximumInlineContentAdsPerHour[] =
    "maximum_inline_content_ads_per_hour";
const int kDefaultMaximumInlineContentAdsPerHour = 4;
const char kFieldTrialParameterMaximumInlineContentAdsPerDay[] =
    "maximum_inline_content_ads_per_day";
const int kDefaultMaximumInlineContentAdsPerDay = 20;

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

const char kFieldTrialParameterBrowsingHistoryMaxCount[] =
    "browsing_history_max_count";
const int kDefaultBrowsingHistoryMaxCount = 5000;
const char kFieldTrialParameterBrowsingHistoryDaysAgo[] =
    "browsing_history_days_ago";
const int kDefaultBrowsingHistoryDaysAgo = 180;

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

int GetMaximumInlineContentAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumInlineContentAdsPerHour,
      kDefaultMaximumInlineContentAdsPerHour);
}

int GetMaximumInlineContentAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumInlineContentAdsPerDay,
      kDefaultMaximumInlineContentAdsPerDay);
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

int GetBrowsingHistoryMaxCount() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterBrowsingHistoryMaxCount,
      kDefaultBrowsingHistoryMaxCount);
}

int GetBrowsingHistoryDaysAgo() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterBrowsingHistoryDaysAgo,
      kDefaultBrowsingHistoryDaysAgo);
}

}  // namespace features
}  // namespace ads
