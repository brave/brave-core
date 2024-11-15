/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_

#include <string_view>

namespace brave_ads {

// Returns true if the condition is an epoch operator.
bool IsEpochOperator(std::string_view condition);

// Matches a value against a condition using epoch operators. Supports equality,
// greater than, greater than or equal, less than. and less than or equal
// operators.
bool MatchEpochOperator(std::string_view value, std::string_view condition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_
