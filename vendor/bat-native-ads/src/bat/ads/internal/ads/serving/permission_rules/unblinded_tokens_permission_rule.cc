/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/unblinded_tokens_permission_rule.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace ads {

namespace {

constexpr int kUnblindedTokensMinimumThreshold = 10;

bool DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  return privacy::UnblindedTokenCount() >= kUnblindedTokensMinimumThreshold;
}

}  // namespace

bool UnblindedTokensPermissionRule::ShouldAllow() {
  if (!DoesRespectCap()) {
    last_message_ = "You do not have enough unblinded tokens";
    return false;
  }

  return true;
}

const std::string& UnblindedTokensPermissionRule::GetLastMessage() const {
  return last_message_;
}

}  // namespace ads
