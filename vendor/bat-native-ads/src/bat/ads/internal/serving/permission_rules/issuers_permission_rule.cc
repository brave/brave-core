/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/issuers_permission_rule.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"

namespace ads {

IssuersPermissionRule::IssuersPermissionRule() = default;

IssuersPermissionRule::~IssuersPermissionRule() = default;

bool IssuersPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "Missing issuers";
    return false;
  }

  return true;
}

std::string IssuersPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool IssuersPermissionRule::DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  if (!IssuerExistsForType(IssuerType::kConfirmations) ||
      !IssuerExistsForType(IssuerType::kPayments)) {
    return false;
  }

  return true;
}

}  // namespace ads
