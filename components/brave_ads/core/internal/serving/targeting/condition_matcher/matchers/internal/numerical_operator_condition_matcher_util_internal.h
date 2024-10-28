/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_

#include <optional>
#include <string_view>

namespace brave_ads {

inline constexpr char kNumericalOperatorConditionMatcherPrefixPattern[] =
    "[R?]:*";

// Parses number from condition.
std::optional<double> ParseNumber(std::string_view condition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_INTERNAL_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_INTERNAL_H_
