/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/allow_notifications_permission_rule.h"

#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"

namespace brave_ads {

namespace {

bool DoesRespectCap() {
  return CanShowNotificationAds();
}

}  // namespace

base::expected<void, std::string>
AllowNotificationsPermissionRule::ShouldAllow() const {
  if (!DoesRespectCap()) {
    return base::unexpected("System notifications not allowed");
  }

  return base::ok();
}

}  // namespace brave_ads
