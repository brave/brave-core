/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/network_connection_permission_rule.h"

#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/ads_client_helper.h"

namespace ads {

namespace {

bool DoesRespectCap() {
  return AdsClientHelper::GetInstance()->IsNetworkConnectionAvailable();
}

}  // namespace

bool NetworkConnectionPermissionRule::ShouldAllow() {
  if (!permission_rules::features::
          ShouldOnlyServeAdsWithValidInternetConnection()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "Network connection is unavailable";
    return false;
  }

  return true;
}

const std::string& NetworkConnectionPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
