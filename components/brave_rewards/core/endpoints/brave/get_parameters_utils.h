/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_UTILS_H_

#include <optional>
#include <string>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"

namespace brave_rewards::internal::endpoints {

std::optional<base::flat_map<std::string, mojom::RegionsPtr>>
GetWalletProviderRegions(const base::Value::Dict&);

}  // namespace brave_rewards::internal::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINTS_BRAVE_GET_PARAMETERS_UTILS_H_
