/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"

#include <cmath>
#include <tuple>

namespace brave_ads {

RewardInfo::RewardInfo() = default;

RewardInfo::RewardInfo(const RewardInfo& other) = default;

RewardInfo& RewardInfo::operator=(const RewardInfo& other) = default;

RewardInfo::RewardInfo(RewardInfo&& other) noexcept = default;

RewardInfo& RewardInfo::operator=(RewardInfo&& other) noexcept = default;

RewardInfo::~RewardInfo() = default;

bool RewardInfo::IsValid() const {
  return token.has_value() && blinded_token.has_value() &&
         unblinded_token.has_value() && public_key.has_value() &&
         !signature.empty() && !credential_base64url.empty();
}

bool operator==(const RewardInfo& lhs, const RewardInfo& rhs) {
  const auto tie = [](const RewardInfo& reward) {
    return std::tie(reward.token, reward.blinded_token, reward.unblinded_token,
                    reward.public_key, reward.signature,
                    reward.credential_base64url);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const RewardInfo& lhs, const RewardInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
