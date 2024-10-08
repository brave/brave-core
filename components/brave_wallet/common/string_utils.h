/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Returns true if a string contains only 0-9 digits
bool IsValidBase10String(std::string_view input);

// Takes a base-10 string as input and converts it to a uint256_t
std::optional<uint256_t> Base10ValueToUint256(std::string_view input);
std::optional<int256_t> Base10ValueToInt256(std::string_view input);

// Takes a uint256_t as input and converts it to a base-10 string
std::string Uint256ValueToBase10(uint256_t input);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_
