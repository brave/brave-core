/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_TEST_UTIL_H_

#include <optional>

namespace brave_ads {

struct ConfirmationInfo;
struct RewardInfo;

namespace test {

std::optional<ConfirmationInfo> BuildRewardConfirmation(
    bool should_generate_random_uuids);

RewardInfo BuildReward(const ConfirmationInfo& confirmation);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_TEST_UTIL_H_
