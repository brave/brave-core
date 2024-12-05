/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/epoch_operator_condition_matcher_util.h"

#include <optional>

#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/epoch_operator_condition_matcher_util_internal.h"

namespace brave_ads {

namespace {

constexpr char kEqualOperatorConditionMatcherPrefix[] = "[T=]:";
constexpr char kNotEqualOperatorConditionMatcherPrefix[] = "[T≠]:";
constexpr char kGreaterThanOperatorConditionMatcherPrefix[] = "[T>]:";
constexpr char kGreaterThanOrEqualOperatorConditionMatcherPrefix[] = "[T≥]:";
constexpr char kLessThanOperatorConditionMatcherPrefix[] = "[T<]:";
constexpr char kLessThanOrEqualOperatorConditionMatcherPrefix[] = "[T≤]:";

}  // namespace

bool IsEpochOperator(std::string_view condition) {
  return base::MatchPattern(condition,
                            kEpochOperatorConditionMatcherPrefixPattern);
}

bool MatchEpochOperator(std::string_view value, std::string_view condition) {
  if (!base::MatchPattern(condition,
                          kEpochOperatorConditionMatcherPrefixPattern)) {
    // Not an operator.
    return false;
  }

  const std::optional<int> days = ParseDays(condition);
  if (!days) {
    // Invalid days.
    return false;
  }

  const std::optional<base::TimeDelta> time_delta = ParseTimeDelta(value);
  if (!time_delta) {
    // Invalid time delta.
    VLOG(1) << "Invalid epoch operator condition matcher for " << condition;
    return false;
  }

  if (condition.starts_with(kEqualOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() == days;
  }

  if (condition.starts_with(kNotEqualOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() != days;
  }

  if (condition.starts_with(kGreaterThanOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() > days;
  }

  if (condition.starts_with(
          kGreaterThanOrEqualOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() >= days;
  }

  if (condition.starts_with(kLessThanOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() < days;
  }

  if (condition.starts_with(kLessThanOrEqualOperatorConditionMatcherPrefix)) {
    return time_delta->InDays() <= days;
  }

  // Unknown operator.
  VLOG(1) << "Unknown epoch operator condition matcher for " << condition;
  return false;
}

}  // namespace brave_ads
