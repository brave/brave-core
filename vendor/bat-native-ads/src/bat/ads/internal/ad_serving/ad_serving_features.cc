/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_serving/ad_serving_features.h"

#include "base/metrics/field_trial_params.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace features {

namespace {

constexpr char kFeatureName[] = "AdServing";

constexpr char kFieldTrialParameterDefaultAdNotificationsPerHour[] =
    "default_ad_notifications_per_hour";
constexpr int kDefaultDefaultAdNotificationsPerHour =
    kDefaultAdNotificationsPerHour;
constexpr char kFieldTrialParameterMaximumAdNotificationsPerDay[] =
    "maximum_ad_notifications_per_day";
constexpr int kDefaultMaximumAdNotificationsPerDay = 40;

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
constexpr int kDefaultBrowsingHistoryMaxCount = 5000;
constexpr char kFieldTrialParameterBrowsingHistoryDaysAgo[] =
    "browsing_history_days_ago";
constexpr int kDefaultBrowsingHistoryDaysAgo = 180;

constexpr char kFieldTrialParameterAdServingVersion[] = "ad_serving_version";
constexpr int kDefaultAdServingVersion = 1;

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

int GetMaximumSearchResultAdsPerHour() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumSearchResultAdsPerHour,
      kDefaultMaximumSearchResultAdsPerHour);
}

int GetMaximumSearchResultAdsPerDay() {
  return GetFieldTrialParamByFeatureAsInt(
      kAdServing, kFieldTrialParameterMaximumSearchResultAdsPerDay,
      kDefaultMaximumSearchResultAdsPerDay);
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

int GetAdServingVersion() {
  return GetFieldTrialParamByFeatureAsInt(kAdServing,
                                          kFieldTrialParameterAdServingVersion,
                                          kDefaultAdServingVersion);
}

}  // namespace features
}  // namespace ads
