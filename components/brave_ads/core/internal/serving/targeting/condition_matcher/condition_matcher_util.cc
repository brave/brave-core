/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>

#include "base/check_op.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/epoch_operator_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/pattern_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/regex_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"

namespace brave_ads {

namespace {

constexpr std::string_view kPrefPathOperatorPrefixPattern = "[?]:*";
constexpr std::string_view kPrefPathNotOperatorPrefix = "[!]:";

std::string_view MaybeStripOperatorPrefix(std::string_view pref_path) {
  if (!base::MatchPattern(pref_path, kPrefPathOperatorPrefixPattern)) {
    // Not an operator.
    return pref_path;
  }

  const size_t pos = pref_path.find(':');
  CHECK_NE(pos, std::string_view::npos);

  return pref_path.substr(pos + 1);
}

bool HasNotOperator(std::string_view pref_path) {
  return pref_path.starts_with(kPrefPathNotOperatorPrefix);
}

bool MatchCondition(const base::DictValue& virtual_prefs,
                    std::string_view pref_path,
                    std::string_view condition) {
  std::string_view stripped_pref_path = MaybeStripOperatorPrefix(pref_path);
  std::optional<std::string> value =
      MaybeGetPrefValueAsString(virtual_prefs, stripped_pref_path);

  if (HasNotOperator(pref_path)) {
    return !value;
  }

  if (std::optional<ConditionMatcherOperatorType> epoch_operator_type =
          MaybeParseEpochOperatorType(condition)) {
    return MatchEpochOperator(
        value.value_or(base::NumberToString(
            base::Time::Now().ToDeltaSinceWindowsEpoch().InMicroseconds())),
        *epoch_operator_type, condition);
  }

  if (std::optional<ConditionMatcherOperatorType> numerical_operator_type =
          MaybeParseNumericalOperatorType(condition)) {
    std::optional<double> numerical_operand =
        MaybeResolveNumericalOperand(condition, virtual_prefs);
    // Missing prefs default to "0".
    return numerical_operand &&
           MatchNumericalOperator(value.value_or("0"), *numerical_operator_type,
                                  *numerical_operand);
  }

  return value &&
         (MatchPattern(*value, condition) || MatchRegex(*value, condition));
}

}  // namespace

bool MatchConditions(const base::DictValue& virtual_prefs,
                     const ConditionMatcherMap& condition_matchers) {
  return std::ranges::all_of(
      condition_matchers, [&virtual_prefs](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;
        return MatchCondition(virtual_prefs, pref_path, condition);
      });
}

}  // namespace brave_ads
