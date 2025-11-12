/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"

#include <limits>
#include <optional>
#include <string_view>

#include "base/numerics/ranges.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/numerical_operator_condition_matcher_util_internal.h"

namespace brave_ads {

namespace {

constexpr std::string_view kEqualOperatorConditionMatcherPrefix = "[R=]:";
constexpr std::string_view kNotEqualOperatorConditionMatcherPrefix = "[R≠]:";
constexpr std::string_view kGreaterThanOperatorConditionMatcherPrefix = "[R>]:";
constexpr std::string_view kGreaterThanOrEqualOperatorConditionMatcherPrefix =
    "[R≥]:";
constexpr std::string_view kLessThanOperatorConditionMatcherPrefix = "[R<]:";
constexpr std::string_view kLessThanOrEqualOperatorConditionMatcherPrefix =
    "[R≤]:";

}  // namespace

bool IsNumericalOperator(std::string_view condition) {
  return base::MatchPattern(condition,
                            kNumericalOperatorConditionMatcherPrefixPattern);
}

bool MatchNumericalOperator(std::string_view value,
                            std::string_view condition) {
  if (!base::MatchPattern(condition,
                          kNumericalOperatorConditionMatcherPrefixPattern)) {
    // Not an operator.
    return false;
  }

  std::optional<double> number = ParseNumber(condition);
  if (!number) {
    // Invalid number.
    return false;
  }

  double value_as_double;
  if (!base::StringToDouble(value, &value_as_double)) {
    // Malformed value.
    BLOG(1, "Malformed numerical operator condition matcher for " << condition);
    return false;
  }

  if (condition.starts_with(kEqualOperatorConditionMatcherPrefix)) {
    return base::IsApproximatelyEqual(value_as_double, *number,
                                      std::numeric_limits<double>::epsilon());
  }

  if (condition.starts_with(kNotEqualOperatorConditionMatcherPrefix)) {
    return !base::IsApproximatelyEqual(value_as_double, *number,
                                       std::numeric_limits<double>::epsilon());
  }

  if (condition.starts_with(kGreaterThanOperatorConditionMatcherPrefix)) {
    return value_as_double > number;
  }

  if (condition.starts_with(
          kGreaterThanOrEqualOperatorConditionMatcherPrefix)) {
    return value_as_double >= number;
  }

  if (condition.starts_with(kLessThanOperatorConditionMatcherPrefix)) {
    return value_as_double < number;
  }

  if (condition.starts_with(kLessThanOrEqualOperatorConditionMatcherPrefix)) {
    return value_as_double <= number;
  }

  // Unknown operator.
  BLOG(1, "Unknown numerical operator condition matcher for " << condition);
  return false;
}

}  // namespace brave_ads
