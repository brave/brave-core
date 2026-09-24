/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"

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

}  // namespace brave_ads
