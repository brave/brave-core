/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/new_tab_page_ads/new_tab_page_ad_permission_rules.h"

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

// static
bool NewTabPageAdPermissionRules::HasPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (!PermissionRulesBase::HasPermission()) {
    return false;
  }

  if (!HasUserActivityPermission()) {
    return false;
  }

  if (!HasCatalogPermission()) {
    return false;
  }

  if (!HasNewTabPageAdsPerDayPermission()) {
    return false;
  }

  if (!HasNewTabPageAdsPerHourPermission()) {
    return false;
  }

  if (!HasNewTabPageAdMinimumWaitTimePermission()) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
