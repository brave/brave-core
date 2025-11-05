/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/condition_matcher_util.h"

#include <algorithm>
#include <cstddef>
#include <optional>

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

constexpr char kPrefPathOperatorPrefixPattern[] = "[?]:*";
constexpr char kPrefPathNotOperatorPrefix[] = "[!]:";

std::string MaybeStripOperatorPrefix(const std::string& pref_path) {
  if (!base::MatchPattern(pref_path, kPrefPathOperatorPrefixPattern)) {
    // Not an operator.
    return pref_path;
  }

  const size_t pos = pref_path.find(':');
  return pref_path.substr(pos + 1);
}

bool HasNotOperator(std::string_view pref_path) {
  return pref_path.starts_with(kPrefPathNotOperatorPrefix);
}

bool MatchCondition(std::string_view value, std::string_view condition) {
  return MatchEpochOperator(value, condition) ||
         MatchNumericalOperator(value, condition) ||
         MatchPattern(value, condition) || MatchRegex(value, condition);
}

}  // namespace

bool MatchConditions(const base::Value::Dict& virtual_prefs,
                     const ConditionMatcherMap& condition_matchers) {
  return std::ranges::all_of(
      condition_matchers, [&virtual_prefs](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;

        const std::string stripped_pref_path =
            MaybeStripOperatorPrefix(pref_path);
        std::optional<std::string> value =
            MaybeGetPrefValueAsString(virtual_prefs, stripped_pref_path);

        if (HasNotOperator(pref_path)) {
          // Match if the pref path does not exist.
          return !value;
        }

        if (IsEpochOperator(condition)) {
          // Default to now if the pref path does not exist.
          return MatchCondition(value.value_or(base::NumberToString(
                                    base::Time::Now()
                                        .ToDeltaSinceWindowsEpoch()
                                        .InMicroseconds())),
                                condition);
        }

        if (IsNumericalOperator(condition)) {
          // Default to "0" if the pref path does not exist.
          return MatchCondition(value.value_or("0"), condition);
        }

        return value && MatchCondition(*value, condition);
      });
}

}  // namespace brave_ads
