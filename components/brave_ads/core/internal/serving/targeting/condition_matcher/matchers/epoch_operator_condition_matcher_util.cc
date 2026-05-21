/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/epoch_operator_condition_matcher_util.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/notreached.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/epoch_operator_condition_matcher_util_internal.h"

namespace brave_ads {

namespace {

constexpr std::string_view kEqualOperatorConditionMatcherPrefix = "[T=]:";
constexpr std::string_view kNotEqualOperatorConditionMatcherPrefix = "[T≠]:";
constexpr std::string_view kGreaterThanOperatorConditionMatcherPrefix = "[T>]:";
constexpr std::string_view kGreaterThanOrEqualOperatorConditionMatcherPrefix =
    "[T≥]:";
constexpr std::string_view kLessThanOperatorConditionMatcherPrefix = "[T<]:";
constexpr std::string_view kLessThanOrEqualOperatorConditionMatcherPrefix =
    "[T≤]:";

}  // namespace

std::optional<ConditionMatcherOperatorType> MaybeParseEpochOperatorType(
    std::string_view condition) {
  if (condition.starts_with(kEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kEqual;
  }

  if (condition.starts_with(kNotEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kNotEqual;
  }

  if (condition.starts_with(kGreaterThanOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kGreaterThan;
  }

  if (condition.starts_with(
          kGreaterThanOrEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kGreaterThanOrEqual;
  }

  if (condition.starts_with(kLessThanOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kLessThan;
  }

  if (condition.starts_with(kLessThanOrEqualOperatorConditionMatcherPrefix)) {
    return ConditionMatcherOperatorType::kLessThanOrEqual;
  }

  return std::nullopt;
}

bool MatchEpochOperator(std::string_view value,
                        ConditionMatcherOperatorType operator_type,
                        std::string_view condition) {
  std::optional<int> days = MaybeParseDays(condition);
  if (!days) {
    // Invalid days.
    return false;
  }

  std::optional<base::TimeDelta> time_delta = MaybeParseTimeDelta(value);
  if (!time_delta) {
    // Invalid time delta.
    BLOG(1, "Invalid epoch operator condition matcher for " << condition);
    return false;
  }

  switch (operator_type) {
    case ConditionMatcherOperatorType::kEqual: {
      return time_delta->InDays() == days;
    }

    case ConditionMatcherOperatorType::kNotEqual: {
      return time_delta->InDays() != days;
    }

    case ConditionMatcherOperatorType::kGreaterThan: {
      return time_delta->InDays() > days;
    }

    case ConditionMatcherOperatorType::kGreaterThanOrEqual: {
      return time_delta->InDays() >= days;
    }

    case ConditionMatcherOperatorType::kLessThan: {
      return time_delta->InDays() < days;
    }

    case ConditionMatcherOperatorType::kLessThanOrEqual: {
      return time_delta->InDays() <= days;
    }
  }

  NOTREACHED() << "Unexpected value for ConditionMatcherOperatorType: "
               << std::to_underlying(operator_type);
}

}  // namespace brave_ads
