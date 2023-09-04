/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CREDENTIAL_JSON_WRITER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CREDENTIAL_JSON_WRITER_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct RewardInfo;

namespace json::writer {

absl::optional<std::string> WriteRewardCredential(
    const absl::optional<RewardInfo>& reward,
    const std::string& payload);

}  // namespace json::writer
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REWARD_REWARD_CREDENTIAL_JSON_WRITER_H_
