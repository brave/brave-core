/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_UTIL_H_

#include <string>

namespace brave_rewards::internal {
namespace sku {

std::string GetBraveDestination(const std::string& wallet_type);

std::string GetUpholdDestination();

std::string GetGeminiDestination();

}  // namespace sku
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SKU_SKU_UTIL_H_
