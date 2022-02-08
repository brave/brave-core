/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_permission_rule.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

namespace ads {

namespace {
const int kUnblindedTokensMinimumThreshold = 10;
}  // namespace

UnblindedTokensPermissionRule::UnblindedTokensPermissionRule() = default;

UnblindedTokensPermissionRule::~UnblindedTokensPermissionRule() = default;

bool UnblindedTokensPermissionRule::ShouldAllow() {
  if (!ShouldRewardUser()) {
    return true;
  }

  if (!DoesRespectCap()) {
    last_message_ = "You do not have enough unblinded tokens";
    return false;
  }

  return true;
}

std::string UnblindedTokensPermissionRule::GetLastMessage() const {
  return last_message_;
}

bool UnblindedTokensPermissionRule::DoesRespectCap() {
  const int count = ConfirmationsState::Get()->get_unblinded_tokens()->Count();
  if (count < kUnblindedTokensMinimumThreshold) {
    return false;
  }

  return true;
}

}  // namespace ads
