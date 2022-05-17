/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_permission_rules.h"

#include "bat/ads/internal/serving/permission_rules/catalog_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/new_tab_page_ads_per_day_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/new_tab_page_ads_per_hour_permission_rule.h"
#include "bat/ads/internal/serving/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/serving/permission_rules/user_activity_permission_rule.h"

namespace ads {
namespace new_tab_page_ads {
namespace frequency_capping {

PermissionRules::PermissionRules() : PermissionRulesBase() {}

PermissionRules::~PermissionRules() = default;

bool PermissionRules::HasPermission() const {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  UserActivityPermissionRule user_activity_permission_rule;
  if (!ShouldAllow(&user_activity_permission_rule)) {
    return false;
  }

  CatalogPermissionRule catalog_permission_rule;
  if (!ShouldAllow(&catalog_permission_rule)) {
    return false;
  }

  NewTabPageAdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(&ads_per_day_permission_rule)) {
    return false;
  }

  NewTabPageAdsPerHourPermissionRule ads_per_hour_permission_rule;
  if (!ShouldAllow(&ads_per_hour_permission_rule)) {
    return false;
  }

  return true;
}

}  // namespace frequency_capping
}  // namespace new_tab_page_ads
}  // namespace ads
