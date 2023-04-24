/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/promoted_content_ads/promoted_content_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/catalog_permission_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_util.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/promoted_content_ads/promoted_content_ads_per_day_permission_rule.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/promoted_content_ads/promoted_content_ads_per_hour_permission_rule.h"

namespace brave_ads {

// static
bool PromotedContentAdPermissionRules::HasPermission() {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  const CatalogPermissionRule catalog_permission_rule;
  if (!ShouldAllow(catalog_permission_rule)) {
    return false;
  }

  const PromotedContentAdsPerDayPermissionRule ads_per_day_permission_rule;
  if (!ShouldAllow(ads_per_day_permission_rule)) {
    return false;
  }

  const PromotedContentAdsPerHourPermissionRule ads_per_hour_permission_rule;
  return ShouldAllow(ads_per_hour_permission_rule);
}

}  // namespace brave_ads
