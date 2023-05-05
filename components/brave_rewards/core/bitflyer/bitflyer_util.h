/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/ledger_types.mojom.h"

namespace brave_rewards::internal::bitflyer {

std::string GetClientId();

std::string GetClientSecret();

std::string GetFeeAddress();

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet);

}  // namespace brave_rewards::internal::bitflyer

namespace brave_rewards::internal::endpoint::bitflyer {

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetServerUrl(const std::string& path);

}  // namespace brave_rewards::internal::endpoint::bitflyer

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_BITFLYER_BITFLYER_UTIL_H_
