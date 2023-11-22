/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/inline_content_ads/inline_content_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

namespace brave_ads {

// static
bool InlineContentAdPermissionRules::HasPermission() {
  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  if (!HasUserActivityPermission()) {
    return false;
  }

  if (!HasCatalogPermission()) {
    return false;
  }

  if (!HasInlineContentAdsPerDayPermission()) {
    return false;
  }

  return HasInlineContentAdsPerHourPermission();
}

}  // namespace brave_ads
