/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_exclusion_rules.h"

#include "bat/ads/ad_info.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/exclusion_rule_util.h"
#include "bat/ads/internal/frequency_capping/exclusion_rules/new_tab_page_ad_uuid_frequency_cap.h"

namespace ads {
namespace new_tab_page_ads {
namespace frequency_capping {

ExclusionRules::ExclusionRules(const AdEventList& ad_events)
    : ad_events_(ad_events) {}

ExclusionRules::~ExclusionRules() = default;

bool ExclusionRules::ShouldExcludeAd(const AdInfo& ad) const {
  NewTabPageAdUuidFrequencyCap frequency_cap(ad_events_);
  return ShouldExclude(ad, &frequency_cap);
}

}  // namespace frequency_capping
}  // namespace new_tab_page_ads
}  // namespace ads
