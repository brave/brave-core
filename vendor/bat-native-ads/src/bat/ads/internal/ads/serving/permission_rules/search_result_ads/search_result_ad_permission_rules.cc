/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/search_result_ads/search_result_ad_permission_rules.h"

#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/ads/serving/permission_rules/search_result_ads/search_result_ads_per_day_permission_rule.h"
#include "bat/ads/internal/ads/serving/permission_rules/search_result_ads/search_result_ads_per_hour_permission_rule.h"

namespace ads {
namespace search_result_ads {

PermissionRules::PermissionRules() : PermissionRulesBase() {}

PermissionRules::~PermissionRules() = default;

bool PermissionRules::HasPermission() const {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  AdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(&ads_per_day_permission_rule)) {
    return false;
  }

  AdsPerHourPermissionRule ads_per_hour_permission_rule;
  return ShouldAllow(&ads_per_hour_permission_rule);
}

}  // namespace search_result_ads
}  // namespace ads
