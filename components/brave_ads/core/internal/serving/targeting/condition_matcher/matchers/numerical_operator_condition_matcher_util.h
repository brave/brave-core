/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_H_

#include <optional>
#include <string_view>

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/condition_matcher_operator_type.h"

namespace base {
class DictValue;
}  // namespace base

// Matches conditions against numeric pref values, all evaluated locally with
// nothing leaving the device. The operand can be a literal number or another
// pref path, allowing ads to be targeted based on relative comparisons between
// two pref values.

namespace brave_ads {

// Tries to parse the numerical operator from a condition string. For example,
// "[R>]:5" returns `ConditionMatcherOperatorType::kGreaterThan`. Returns
// `std::nullopt` if `condition` does not contain a numerical operator.
std::optional<ConditionMatcherOperatorType> MaybeParseNumericalOperatorType(
    std::string_view condition);

// Tries to resolve the operand for a numerical operator condition. Returns the
// value directly if it is a number, or resolves it as a pref path. Returns
// `std::nullopt` if it cannot be resolved to a numeric value.
std::optional<double> MaybeResolveNumericalOperand(
    std::string_view condition,
    const base::DictValue& virtual_prefs);

// Matches a value against a numerical condition using equality, greater than,
// greater than or equal, less than, less than or equal, and not equal. The >=
// and <= checks use a small absolute epsilon. This is acceptable here because
// operands are parsed from numeric strings.
bool MatchNumericalOperator(std::string_view value,
                            ConditionMatcherOperatorType operator_type,
                            double operand);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_NUMERICAL_OPERATOR_CONDITION_MATCHER_UTIL_H_
