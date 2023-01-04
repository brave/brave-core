/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/issuers_permission_rule.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"

namespace ads {

namespace {

bool DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  return HasIssuers();
}

}  // namespace

bool IssuersPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Missing issuers";
    return false;
  }

  return true;
}

const std::string& IssuersPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
