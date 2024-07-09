/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/confirmation_tokens_permission_rule.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"

namespace brave_ads {

namespace {
constexpr size_t kMinimumConfirmationTokenThreshold = 10;
}  // namespace

bool HasConfirmationTokensPermission() {
  if (!UserHasJoinedBraveRewards()) {
    return true;
  }

  if (ConfirmationTokenCount() >= kMinimumConfirmationTokenThreshold) {
    return true;
  }

  BLOG(2, "You do not have enough confirmation tokens");
  return false;
}

}  // namespace brave_ads
