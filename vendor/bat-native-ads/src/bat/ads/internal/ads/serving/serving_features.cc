/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/serving_features.h"

#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"
#include "bat/ads/internal/common/metrics/field_trial_params_util.h"
#include "brave/components/brave_ads/common/constants.h"

namespace ads::features {

namespace {

constexpr char kFeatureName[] = "AdServing";

constexpr char kFieldTrialParameterDefaultNotificationAdsPerHour[] =
    "default_ad_notifications_per_hour";
constexpr int kDefaultDefaultNotificationAdsPerHour =
    kDefaultNotificationAdsPerHour;
constexpr char kFieldTrialParameterMaximumNotificationAdsPerDay[] =
    "maximum_ad_notifications_per_day";
constexpr int kDefaultMaximumNotificationAdsPerDay = 40;

constexpr char kFieldTrialParameterMaximumInlineContentAdsPerHour[] =
    "maximum_inline_content_ads_per_hour";
constexpr int kDefaultMaximumInlineContentAdsPerHour = 4;
constexpr char kFieldTrialParameterMaximumInlineContentAdsPerDay[] =
    "maximum_inline_content_ads_per_day";
constexpr int kDefaultMaximumInlineContentAdsPerDay = 20;

constexpr char kFieldTrialParameterMaximumNewTabPageAdsPerHour[] =
    "maximum_new_tab_page_ads_per_hour";
constexpr int kDefaultMaximumNewTabPageAdsPerHour = 4;
constexpr char kFieldTrialParameterMaximumNewTabPageAdsPerDay[] =
    "maximum_new_tab_page_ads_per_day";
constexpr int kDefaultMaximumNewTabPageAdsPerDay = 20;

constexpr char kFieldTrialParameterNewTabPageAdsMinimumWaitTime[] =
    "new_tab_page_ads_minimum_wait_time";
constexpr base::TimeDelta kDefaultNewTabPageAdsMinimumWaitTime =
    base::Minutes(5);

constexpr char kFieldTrialParameterMaximumPromotedContentAdsPerHour[] =
    "maximum_promoted_content_ads_per_hour";
constexpr int kDefaultMaximumPromotedContentAdsPerHour = 4;
constexpr char kFieldTrialParameterMaximumPromotedContentAdsPerDay[] =
    "maximum_promoted_content_ads_per_day";
constexpr int kDefaultMaximumPromotedContentAdsPerDay = 20;

constexpr char kFieldTrialParameterMaximumSearchResultAdsPerHour[] =
    "maximum_search_result_ads_per_hour";
constexpr int kDefaultMaximumSearchResultAdsPerHour = 10;
constexpr char kFieldTrialParameterMaximumSearchResultAdsPerDay[] =
    "maximum_search_result_ads_per_day";
constexpr int kDefaultMaximumSearchResultAdsPerDay = 40;

constexpr char kFieldTrialParameterBrowsingHistoryMaxCount[] =
    "browsing_history_max_count";
constexpr int kDefaultBrowsingHistoryMaxCount = 5'000;
constexpr char kFieldTrialParameterBrowsingHistoryDaysAgo[] =
    "browsing_history_days_ago";
constexpr int kDefaultBrowsingHistoryDaysAgo = 180;

constexpr char kFieldTrialParameterServingVersion[] = "ad_serving_version";
constexpr int kDefaultServingVersion = 1;

}  // namespace

BASE_FEATURE(kServing, kFeatureName, base::FEATURE_ENABLED_BY_DEFAULT);

bool IsServingEnabled() {
  return base::FeatureList::IsEnabled(kServing);
}

int GetServingVersion() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterServingVersion, kDefaultServingVersion);
}

int GetDefaultNotificationAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterDefaultNotificationAdsPerHour,
      kDefaultDefaultNotificationAdsPerHour);
}

int GetMaximumNotificationAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumNotificationAdsPerDay,
      kDefaultMaximumNotificationAdsPerDay);
}

int GetMaximumInlineContentAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumInlineContentAdsPerHour,
      kDefaultMaximumInlineContentAdsPerHour);
}

int GetMaximumInlineContentAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumInlineContentAdsPerDay,
      kDefaultMaximumInlineContentAdsPerDay);
}

int GetMaximumNewTabPageAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumNewTabPageAdsPerHour,
      kDefaultMaximumNewTabPageAdsPerHour);
}

int GetMaximumNewTabPageAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumNewTabPageAdsPerDay,
      kDefaultMaximumNewTabPageAdsPerDay);
}

base::TimeDelta GetNewTabPageAdsMinimumWaitTime() {
  return GetFieldTrialParamByFeatureAsTimeDelta(
      kServing, kFieldTrialParameterNewTabPageAdsMinimumWaitTime,
      kDefaultNewTabPageAdsMinimumWaitTime);
}

int GetMaximumPromotedContentAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumPromotedContentAdsPerHour,
      kDefaultMaximumPromotedContentAdsPerHour);
}

int GetMaximumPromotedContentAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumPromotedContentAdsPerDay,
      kDefaultMaximumPromotedContentAdsPerDay);
}

int GetMaximumSearchResultAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumSearchResultAdsPerHour,
      kDefaultMaximumSearchResultAdsPerHour);
}

int GetMaximumSearchResultAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterMaximumSearchResultAdsPerDay,
      kDefaultMaximumSearchResultAdsPerDay);
}

int GetBrowsingHistoryMaxCount() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterBrowsingHistoryMaxCount,
      kDefaultBrowsingHistoryMaxCount);
}

int GetBrowsingHistoryDaysAgo() {
  return GetFieldTrialParamByFeatureAsInt(
      kServing, kFieldTrialParameterBrowsingHistoryDaysAgo,
      kDefaultBrowsingHistoryDaysAgo);
}

}  // namespace ads::features
