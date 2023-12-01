/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UTIL_H_

#include <optional>
#include <string>
#include <utility>

#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::cbr {

template <typename T, typename U = T>
std::optional<U> ValueOrLogError(base::expected<T, std::string> result) {
  if (!result.has_value()) {
    BLOG(1, "Challenge Bypass Ristretto Error: " << result.error());
    return std::nullopt;
  }
  return std::make_optional<U>(std::move(result).value());
}

}  // namespace brave_ads::cbr

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CHALLENGE_BYPASS_RISTRETTO_CHALLENGE_BYPASS_RISTRETTO_UTIL_H_
