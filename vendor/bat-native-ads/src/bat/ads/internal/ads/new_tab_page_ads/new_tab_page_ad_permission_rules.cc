/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_permission_rules.h"

#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"

namespace ads {
namespace new_tab_page_ads {
namespace frequency_capping {

PermissionRules::PermissionRules() : PermissionRulesBase() {}

PermissionRules::~PermissionRules() = default;

bool PermissionRules::HasPermission() const {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  NewTabPageAdsPerDayFrequencyCap ads_per_day_frequency_cap;
  if (!ShouldAllow(&ads_per_day_frequency_cap)) {
    return false;
  }

  NewTabPageAdsPerHourFrequencyCap ads_per_hour_frequency_cap;
  if (!ShouldAllow(&ads_per_hour_frequency_cap)) {
    return false;
  }

  return true;
}

}  // namespace frequency_capping
}  // namespace new_tab_page_ads
}  // namespace ads
