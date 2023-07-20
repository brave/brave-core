/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/unblinded_tokens_permission_rule.h"

#include "brave/components/brave_ads/core/internal/account/account_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"

namespace brave_ads {

namespace {

constexpr int kUnblindedTokensMinimumThreshold = 10;

bool DoesRespectCap() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  return privacy::UnblindedTokenCount() >= kUnblindedTokensMinimumThreshold;
}

}  // namespace

base::expected<void, std::string> UnblindedTokensPermissionRule::ShouldAllow()
    const {
  if (!DoesRespectCap()) {
    return base::unexpected("You do not have enough unblinded tokens");
  }

  return base::ok();
}

}  // namespace brave_ads
