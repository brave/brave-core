/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/ad_notifications/ad_notifications_frequency_capping.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/conversion_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daily_cap_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/daypart_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/dismissed_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_as_inappropriate_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/marked_to_no_longer_receive_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/subdivision_targeting_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/total_max_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/transferred_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/allow_notifications_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/browser_is_active_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/do_not_disturb_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/media_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/minimum_wait_time_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/network_connection_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

FrequencyCapping::FrequencyCapping(
    AdsImpl* ads,
    const AdEventList& ad_events)
    : ads_(ads),
      ad_events_(ad_events) {
  DCHECK(ads_);
}

FrequencyCapping::~FrequencyCapping() = default;

bool FrequencyCapping::IsAdAllowed() {
  AllowNotificationsFrequencyCap allow_notifications_frequency_cap(ads_);
  if (!ShouldAllow(&allow_notifications_frequency_cap)) {
    return false;
  }

  NetworkConnectionFrequencyCap network_connection_frequency_cap(ads_);
  if (!ShouldAllow(&network_connection_frequency_cap)) {
    return false;
  }

  BrowserIsActiveFrequencyCap browser_is_active_frequency_cap(ads_);
  if (!ShouldAllow(&browser_is_active_frequency_cap)) {
    return false;
  }

  DoNotDisturbFrequencyCap do_not_disturb_frequency_cap(ads_);
  if (!ShouldAllow(&do_not_disturb_frequency_cap)) {
    return false;
  }

  CatalogFrequencyCap catalog_frequency_cap(ads_);
  if (!ShouldAllow(&catalog_frequency_cap)) {
    return false;
  }

  UnblindedTokensFrequencyCap unblinded_tokens_frequency_cap(ads_);
  if (!ShouldAllow(&unblinded_tokens_frequency_cap)) {
    return false;
  }

  UserActivityFrequencyCap user_activity_frequency_cap(ads_);
  if (!ShouldAllow(&user_activity_frequency_cap)) {
    return false;
  }

  MediaFrequencyCap media_frequency_cap(ads_);
  if (!ShouldAllow(&media_frequency_cap)) {
    return false;
  }

  AdsPerDayFrequencyCap ads_per_day_frequency_cap(ads_, ad_events_);
  if (!ShouldAllow(&ads_per_day_frequency_cap)) {
    return false;
  }

  AdsPerHourFrequencyCap ads_per_hour_frequency_cap(ads_, ad_events_);
  if (!ShouldAllow(&ads_per_hour_frequency_cap)) {
    return false;
  }

  MinimumWaitTimeFrequencyCap minimum_wait_time_frequency_cap(ads_, ad_events_);
  if (!ShouldAllow(&minimum_wait_time_frequency_cap)) {
    return false;
  }

  return true;
}

bool FrequencyCapping::ShouldExcludeAd(
    const CreativeAdInfo& ad) {
  bool should_exclude = false;

  DailyCapFrequencyCap daily_cap_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &daily_cap_frequency_cap)) {
    should_exclude = true;
  }

  PerDayFrequencyCap per_day_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &per_day_frequency_cap)) {
    should_exclude = true;
  }

  PerHourFrequencyCap per_hour_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &per_hour_frequency_cap)) {
    should_exclude = true;
  }

  TotalMaxFrequencyCap total_max_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &total_max_frequency_cap)) {
    should_exclude = true;
  }

  ConversionFrequencyCap conversion_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &conversion_frequency_cap)) {
    should_exclude = true;
  }

  SubdivisionTargetingFrequencyCap subdivision_frequency_cap(ads_);
  if (ShouldExclude(ad, &subdivision_frequency_cap)) {
    should_exclude = true;
  }

  DaypartFrequencyCap daypart_frequency_cap(ads_);
  if (ShouldExclude(ad, &daypart_frequency_cap)) {
    should_exclude = true;
  }

  DismissedFrequencyCap dismissed_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &dismissed_frequency_cap)) {
    should_exclude = true;
  }

  TransferredFrequencyCap transferred_frequency_cap(ads_, ad_events_);
  if (ShouldExclude(ad, &transferred_frequency_cap)) {
    should_exclude = true;
  }

  MarkedToNoLongerReceiveFrequencyCap
      marked_to_no_longer_receive_frequency_cap(ads_);
  if (ShouldExclude(ad, &marked_to_no_longer_receive_frequency_cap)) {
    should_exclude = true;
  }

  MarkedAsInappropriateFrequencyCap marked_as_inappropriate_frequency_cap(ads_);
  if (ShouldExclude(ad, &marked_as_inappropriate_frequency_cap)) {
    should_exclude = true;
  }

  return should_exclude;
}

}  // namespace ad_notifications
}  // namespace ads
