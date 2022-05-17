/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/allow_notifications_permission_rule.h"

#include "bat/ads/internal/ads_client_helper.h"

namespace ads {

AllowNotificationsPermissionRule::AllowNotificationsPermissionRule() = default;

AllowNotificationsPermissionRule::~AllowNotificationsPermissionRule() = default;

bool AllowNotificationsPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Notifications not allowed";
    return false;
  }

  return true;
}

std::string AllowNotificationsPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool AllowNotificationsPermissionRule::DoesRespectCap() {
  return AdsClientHelper::Get()->ShouldShowNotifications();
}

}  // namespace ads
