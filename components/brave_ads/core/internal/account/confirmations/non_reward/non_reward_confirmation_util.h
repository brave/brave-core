/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_NON_REWARD_NON_REWARD_CONFIRMATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_NON_REWARD_NON_REWARD_CONFIRMATION_UTIL_H_

#include <optional>

namespace brave_ads {

struct ConfirmationInfo;
struct TransactionInfo;
struct UserDataInfo;

std::optional<ConfirmationInfo> BuildNonRewardConfirmation(
    const TransactionInfo& transaction,
    const UserDataInfo& user_data);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_NON_REWARD_NON_REWARD_CONFIRMATION_UTIL_H_
