/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"

#include <limits>
#include <optional>
#include <string_view>

#include "base/numerics/ranges.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"

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

std::optional<ConditionMatcherNumericalOperatorType>
MaybeGetNumericalOperatorType(std::string_view condition) {
  if (condition.starts_with(kEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kEqual;
  }

  if (condition.starts_with(kNotEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kNotEqual;
  }

  if (condition.starts_with(kGreaterThanOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kGreaterThan;
  }

  if (condition.starts_with(
          kGreaterThanOrEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual;
  }

  if (condition.starts_with(kLessThanOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kLessThan;
  }

  if (condition.starts_with(kLessThanOrEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherNumericalOperatorType::kLessThanOrEqual;
  }

  return std::nullopt;
}

std::optional<double> MaybeResolveNumericalOperand(
    std::string_view condition,
    const base::DictValue& virtual_prefs) {
  const size_t pos = condition.find(':');
  if (pos == std::string_view::npos || pos + 1 >= condition.size()) {
    BLOG(1, "Malformed numerical operator condition matcher for " << condition);
    return std::nullopt;
  }

  std::string_view numerical_operand = condition.substr(pos + 1);

  double numerical_operand_as_double;
  if (base::StringToDouble(numerical_operand, &numerical_operand_as_double)) {
    // Operand is already a number.
    return numerical_operand_as_double;
  }

  // Otherwise, resolve operand as a pref path.
  std::optional<std::string> pref_value =
      MaybeGetPrefValueAsString(virtual_prefs, /*pref_path=*/numerical_operand);
  if (!pref_value) {
    BLOG(1, "Unknown pref path in numerical operator condition matcher for "
                << condition);
    return std::nullopt;
  }

  if (!base::StringToDouble(*pref_value, &numerical_operand_as_double)) {
    BLOG(1,
         "Non-numeric pref path value in numerical operator condition "
         "matcher for "
             << condition);
    return std::nullopt;
  }

  return numerical_operand_as_double;
}

bool MatchNumericalOperator(std::string_view value,
                            ConditionMatcherNumericalOperatorType operator_type,
                            double operand) {
  double value_as_double;
  if (!base::StringToDouble(value, &value_as_double)) {
    BLOG(1, "Malformed numerical operator condition matcher for " << value);
    return false;
  }

  switch (operator_type) {
    case ConditionMatcherNumericalOperatorType::kEqual: {
      return base::IsApproximatelyEqual(value_as_double, operand,
                                        std::numeric_limits<double>::epsilon());
    }

    case ConditionMatcherNumericalOperatorType::kNotEqual: {
      return !base::IsApproximatelyEqual(
          value_as_double, operand, std::numeric_limits<double>::epsilon());
    }

    case ConditionMatcherNumericalOperatorType::kGreaterThan: {
      return value_as_double > operand;
    }

    case ConditionMatcherNumericalOperatorType::kGreaterThanOrEqual: {
      return value_as_double > operand ||
             base::IsApproximatelyEqual(value_as_double, operand,
                                        std::numeric_limits<double>::epsilon());
    }

    case ConditionMatcherNumericalOperatorType::kLessThan: {
      return value_as_double < operand;
    }

    case ConditionMatcherNumericalOperatorType::kLessThanOrEqual: {
      return value_as_double < operand ||
             base::IsApproximatelyEqual(value_as_double, operand,
                                        std::numeric_limits<double>::epsilon());
    }
  }

  return false;
}

}  // namespace brave_ads
