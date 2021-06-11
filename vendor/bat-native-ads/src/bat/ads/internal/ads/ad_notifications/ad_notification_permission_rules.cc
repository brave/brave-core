/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification_permission_rules.h"

#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/allow_notifications_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/browser_is_active_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/catalog_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/do_not_disturb_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/full_screen_mode_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/media_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/minimum_wait_time_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/network_connection_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/permission_rule_util.h"
#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/user_activity_frequency_cap.h"

namespace ads {
namespace ad_notifications {
namespace frequency_capping {

PermissionRules::PermissionRules() = default;

PermissionRules::~PermissionRules() = default;

bool PermissionRules::HasPermission() const {
  AllowNotificationsFrequencyCap allow_notifications_frequency_cap;
  if (!ShouldAllow(&allow_notifications_frequency_cap)) {
    return false;
  }

  NetworkConnectionFrequencyCap network_connection_frequency_cap;
  if (!ShouldAllow(&network_connection_frequency_cap)) {
    return false;
  }

  FullScreenModeFrequencyCap full_screen_mode_frequency_cap;
  if (!ShouldAllow(&full_screen_mode_frequency_cap)) {
    return false;
  }

  BrowserIsActiveFrequencyCap browser_is_active_frequency_cap;
  if (!ShouldAllow(&browser_is_active_frequency_cap)) {
    return false;
  }

  DoNotDisturbFrequencyCap do_not_disturb_frequency_cap;
  if (!ShouldAllow(&do_not_disturb_frequency_cap)) {
    return false;
  }

  CatalogFrequencyCap catalog_frequency_cap;
  if (!ShouldAllow(&catalog_frequency_cap)) {
    return false;
  }

  UnblindedTokensFrequencyCap unblinded_tokens_frequency_cap;
  if (!ShouldAllow(&unblinded_tokens_frequency_cap)) {
    return false;
  }

  UserActivityFrequencyCap user_activity_frequency_cap;
  if (!ShouldAllow(&user_activity_frequency_cap)) {
    return false;
  }

  MediaFrequencyCap media_frequency_cap;
  if (!ShouldAllow(&media_frequency_cap)) {
    return false;
  }

  AdsPerDayFrequencyCap ads_per_day_frequency_cap;
  if (!ShouldAllow(&ads_per_day_frequency_cap)) {
    return false;
  }

  AdsPerHourFrequencyCap ads_per_hour_frequency_cap;
  if (!ShouldAllow(&ads_per_hour_frequency_cap)) {
    return false;
  }

  MinimumWaitTimeFrequencyCap minimum_wait_time_frequency_cap;
  if (!ShouldAllow(&minimum_wait_time_frequency_cap)) {
    return false;
  }

  return true;
}

}  // namespace frequency_capping
}  // namespace ad_notifications
}  // namespace ads
