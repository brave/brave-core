/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_unittest_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token_unittest_util.h"

namespace brave_ads {

RewardInfo BuildRewardForTesting() {
  RewardInfo reward;

  reward.token = privacy::cbr::GetTokenForTesting();
  reward.blinded_token = privacy::cbr::GetBlindedTokenForTesting();
  reward.unblinded_token = privacy::cbr::GetUnblindedTokenForTesting();
  reward.public_key = privacy::cbr::GetPublicKeyForTesting();
  reward.signature = "signature";
  reward.credential_base64url = "credential";

  return reward;
}

}  // namespace brave_ads
