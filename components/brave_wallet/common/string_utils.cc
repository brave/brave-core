/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/string_utils.h"

#include <limits>
#include <string>
#include <string_view>

#include "base/numerics/safe_math.h"
#include "base/strings/string_number_conversions_internal.h"
#include "base/strings/string_util.h"

namespace brave_wallet {

std::optional<uint256_t> Base10ValueToUint256(std::string_view input) {
  uint256_t output;
  return base::internal::StringToIntImpl(input, output) ? std::optional(output)
                                                        : std::nullopt;
}

std::optional<int256_t> Base10ValueToInt256(std::string_view input) {
  int256_t output;
  return base::internal::StringToIntImpl(input, output) ? std::optional(output)
                                                        : std::nullopt;
}

std::string Uint256ValueToBase10(uint256_t value) {
  return base::internal::IntToStringT<std::string>(value);
}

}  // namespace brave_wallet
