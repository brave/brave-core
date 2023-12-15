/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards::internal::zebpay {

std::string GetLoginUrl(const std::string& state);

std::string GetClientId();

std::string GetClientSecret();

std::string GetAccountUrl();

std::string GetActivityUrl();

}  // namespace brave_rewards::internal::zebpay

namespace brave_rewards::internal::endpoint::zebpay {

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetApiServerUrl(const std::string& path);
std::string GetOauthServerUrl(const std::string& path);

}  // namespace brave_rewards::internal::endpoint::zebpay

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ZEBPAY_ZEBPAY_UTIL_H_
