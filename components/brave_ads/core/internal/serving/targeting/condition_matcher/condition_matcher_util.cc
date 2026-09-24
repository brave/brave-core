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

// Epoch ("[T...]:") and numerical ("[R...]:") operators only recognize a
// single operator character between the letter and "]:" (e.g. "[T≥]:",
// never "[T>=]:"). Neither `MaybeParseEpochOperatorType` nor
// `MaybeParseNumericalOperatorType` matches a malformed multi-character
// attempt like that, so without this check it would silently fall through
// to the Pattern/Regex matcher and be treated as a literal string to match
// against, rather than flagged as the mistake it almost certainly is.
bool LooksLikeMalformedOperatorPrefix(std::string_view condition) {
  return (condition.starts_with("[T") || condition.starts_with("[R")) &&
         condition.contains("]:");
}

}  // namespace

bool HasNotOperator(std::string_view pref_path) {
  return pref_path.starts_with(kPrefPathNotOperatorPrefix);
}

ConditionMatchResult MatchCondition(const base::DictValue& virtual_prefs,
                                    std::string_view pref_path,
                                    std::string_view condition) {
  return MatchCondition(virtual_prefs, pref_path, condition,
                        /*test_value=*/std::nullopt);
}

ConditionMatchResult MatchCondition(const base::DictValue& virtual_prefs,
                                    std::string_view pref_path,
                                    std::string_view condition,
                                    std::optional<std::string> test_value) {
  std::string_view stripped_pref_path = MaybeStripOperatorPrefix(pref_path);
  std::optional<std::string> value =
      test_value ? std::move(test_value)
                 : MaybeGetPrefValueAsString(virtual_prefs, stripped_pref_path);

  if (HasNotOperator(pref_path)) {
    return !value ? ConditionMatchResult::kMatch
                  : ConditionMatchResult::kNoMatch;
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
    if (!numerical_operand) {
      return ConditionMatchResult::kInvalid;
    }
    // Missing prefs default to "0".
    return MatchNumericalOperator(value.value_or("0"), *numerical_operator_type,
                                  *numerical_operand);
  }

  if (LooksLikeMalformedOperatorPrefix(condition)) {
    return ConditionMatchResult::kInvalid;
  }

  // Pattern and regex conditions never return invalid. A leading or dangling
  // "*" is a valid glob pattern but invalid RE2 syntax, so a failed regex
  // compile is expected here, not a sign of a malformed condition.
  //
  // Regex is only tried when the condition uses syntax that only means
  // something in regex, e.g. "^$|(){}[]\". This takes priority over a glob
  // wildcard ("*"/"?") also being present, since "^1\.*" uses "*" as a
  // regex quantifier, not a glob wildcard. Otherwise, glob is used, since
  // RE2::PartialMatch is an unanchored substring search and would match far
  // more loosely than intended. For example "1.*" as a glob only matches
  // values starting with "1.", but as a loose regex it would also match
  // "152.1.95.0". A bare literal like "M" is matched exactly for the same
  // reason, not as a substring.
  const bool has_regex_only_syntax =
      condition.find_first_of("^$|(){}[]\\") != std::string_view::npos;

  return value && (has_regex_only_syntax ? MatchRegex(*value, condition)
                                         : MatchPattern(*value, condition))
             ? ConditionMatchResult::kMatch
             : ConditionMatchResult::kNoMatch;
}

bool MatchConditions(const base::DictValue& virtual_prefs,
                     const ConditionMatcherMap& condition_matchers) {
  return std::ranges::all_of(
      condition_matchers, [&virtual_prefs](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;
        return MatchCondition(virtual_prefs, pref_path, condition) ==
               ConditionMatchResult::kMatch;
      });
}

}  // namespace brave_ads
