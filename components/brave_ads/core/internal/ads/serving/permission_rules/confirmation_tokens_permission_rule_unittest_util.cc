/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/confirmation_tokens_permission_rule_unittest_util.h"

#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"

namespace brave_ads {

void ForceConfirmationTokensPermissionRuleForTesting() {
  privacy::SetConfirmationTokens(/*count*/ 50);
}

}  // namespace brave_ads
