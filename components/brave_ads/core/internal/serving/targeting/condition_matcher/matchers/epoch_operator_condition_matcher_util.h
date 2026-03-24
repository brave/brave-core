/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/condition_matcher_operator_type.h"

// Matches conditions based on when an event occurred relative to now, allowing
// ads to be targeted based on time-sensitive criteria, all evaluated locally
// with nothing leaving the device. Timestamps stored in prefs may be
// Unix, Windows, or ISO 8601 format; format detection is handled internally.

namespace brave_ads {

// Tries to parse the epoch operator from a condition string. For example,
// "[T>]:3" returns `ConditionMatcherOperatorType::kGreaterThan`. Returns
// `std::nullopt` if `condition` does not contain an epoch operator.
std::optional<ConditionMatcherOperatorType> MaybeParseEpochOperatorType(
    std::string_view condition);

// Matches a value against an epoch condition using equality, greater than,
// greater than or equal, less than, and less than or equal operators.
bool MatchEpochOperator(std::string_view value,
                        ConditionMatcherOperatorType operator_type,
                        std::string_view condition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_EPOCH_OPERATOR_CONDITION_MATCHER_UTIL_H_
