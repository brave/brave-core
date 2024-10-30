/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/targeting/condition_matcher/condition_matcher_util.h"

#include <cstddef>
#include <optional>

#include "base/ranges/algorithm.h"
#include "base/strings/pattern.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/epoch_operator_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/numerical_operator_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/pattern_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/regex_condition_matcher_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

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

bool HasNotOperator(const std::string_view pref_path) {
  return pref_path.starts_with(kPrefPathNotOperatorPrefix);
}

bool MatchCondition(const std::string_view value,
                    const std::string_view condition) {
  return MatchEpochOperator(value, condition) ||
         MatchNumericalOperator(value, condition) ||
         MatchPattern(value, condition) || MatchRegex(value, condition);
}

}  // namespace

bool MatchConditions(const PrefProviderInterface* const pref_provider,
                     const ConditionMatcherMap& condition_matchers) {
  CHECK(pref_provider);

  return base::ranges::all_of(
      condition_matchers, [pref_provider](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;

        // If `has_not_operator` is `true`, the condition will match only if the
        // pref path does not exist.
        const bool has_not_operator = HasNotOperator(pref_path);

        const std::string stripped_pref_path =
            MaybeStripOperatorPrefix(pref_path);
        if (const std::optional<std::string> value =
                MaybeGetPrefValueAsString(pref_provider, stripped_pref_path)) {
          return !has_not_operator && MatchCondition(*value, condition);
        }

        // Unknown pref path.
        return has_not_operator;
      });
}

}  // namespace brave_ads
