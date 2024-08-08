/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_UTIL_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace brave_ads {

struct ConfirmationInfo;
struct TransactionInfo;

std::optional<std::string> BuildRewardCredential(
    const ConfirmationInfo& confirmation);

std::optional<ConfirmationInfo> BuildRewardConfirmation(
    const TransactionInfo& transaction,
    base::Value::Dict user_data);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CONFIRMATION_UTIL_H_
