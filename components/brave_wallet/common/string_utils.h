/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace brave_wallet {

// Returns true if a string contains only 0-9 digits
bool IsValidBase10String(const std::string& input);

// Takes a base-10 string as input and converts it to a uint256_t
bool Base10ValueToUint256(const std::string& input, uint256_t* out);
bool Base10ValueToInt256(const std::string& input, int256_t* out);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_STRING_UTILS_H_
