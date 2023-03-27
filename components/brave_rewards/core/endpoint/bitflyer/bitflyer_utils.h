/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_UTILS_H_

#include <string>
#include <vector>

namespace brave_rewards::core {
namespace endpoint {
namespace bitflyer {

std::string GetClientId();

std::string GetClientSecret();

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetServerUrl(const std::string& path);

}  // namespace bitflyer
}  // namespace endpoint
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_BITFLYER_BITFLYER_UTILS_H_
