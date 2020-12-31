/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/new_tab_page_ads/new_tab_page_ads_frequency_capping.h"

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/new_tab_page_ad_uuid_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace new_tab_page_ads {

FrequencyCapping::FrequencyCapping(
    const AdEventList& ad_events)
    : ad_events_(ad_events) {
}

FrequencyCapping::~FrequencyCapping() = default;

bool FrequencyCapping::IsAdAllowed() {
  UnblindedTokensFrequencyCap unblinded_tokens_frequency_cap;
  if (!ShouldAllow(&unblinded_tokens_frequency_cap)) {
    return false;
  }

  NewTabPageAdsPerDayFrequencyCap ads_per_day_frequency_cap(ad_events_);
  if (!ShouldAllow(&ads_per_day_frequency_cap)) {
    return false;
  }

  NewTabPageAdsPerHourFrequencyCap ads_per_hour_frequency_cap(ad_events_);
  if (!ShouldAllow(&ads_per_hour_frequency_cap)) {
    return false;
  }

  return true;
}

bool FrequencyCapping::ShouldExcludeAd(
    const AdInfo& ad) {
  NewTabPageAdUuidFrequencyCap frequency_cap(ad_events_);
  return ShouldExclude(ad, &frequency_cap);
}

}  // namespace new_tab_page_ads
}  // namespace ads
