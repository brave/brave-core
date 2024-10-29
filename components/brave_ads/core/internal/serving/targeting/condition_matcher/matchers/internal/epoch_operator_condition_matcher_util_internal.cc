/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/matchers/internal/epoch_operator_condition_matcher_util_internal.h"

#include <cstddef>
#include <limits>
#include <string>

#include "base/check.h"
#include "base/logging.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

namespace brave_ads {

namespace {
constexpr int32_t kMaxUnixEpochTimestamp = std::numeric_limits<int32_t>::max();
}  // namespace

std::optional<int> ParseDays(const std::string_view condition) {
  CHECK(base::MatchPattern(condition,
                           kEpochOperatorConditionMatcherPrefixPattern));

  const size_t pos = condition.find(':');
  if (pos == std::string::npos || pos + 1 >= condition.size()) {
    // Malformed operator.
    VLOG(1) << "Malformed epoch operator condition matcher for " << condition;
    return std::nullopt;
  }

  int days;
  if (!base::StringToInt(condition.substr(pos + 1), &days)) {
    // Malformed days.
    VLOG(1) << "Malformed epoch operator condition matcher for " << condition;
    return std::nullopt;
  }

  if (days < 0) {
    VLOG(1) << "Invalid epoch operator condition matcher for " << condition;
    return std::nullopt;
  }

  return days;
}

bool IsUnixEpochTimestamp(const int64_t timestamp) {
  // 32-bit Unix epoch timestamps will fail in the Year 2038 (Y2038K), whereas
  // Windows epoch timestamps are 64-bit and will not fail within a foreseeable
  // timeframe. We should support Unix epoch timestamps that were not serialized
  // using `base::Time::ToDeltaSinceWindowsEpoch`.
  return timestamp >= 0 && timestamp <= kMaxUnixEpochTimestamp;
}

int64_t WindowsToUnixEpoch(const int64_t timestamp) {
  return (timestamp - base::Time::kTimeTToMicrosecondsOffset) /
         base::Time::kMicrosecondsPerSecond;
}

base::TimeDelta TimeDeltaSinceEpoch(const int64_t timestamp) {
  base::Time now = base::Time::Now();

  if (!IsUnixEpochTimestamp(timestamp)) {
    return now - base::Time::FromDeltaSinceWindowsEpoch(
                     base::Microseconds(timestamp));
  }

  return now -
         base::Time::FromSecondsSinceUnixEpoch(static_cast<double>(timestamp));
}

std::optional<base::TimeDelta> ParseTimeDelta(const std::string_view value) {
  double timestamp;
  if (base::StringToDouble(value, &timestamp)) {
    return TimeDeltaSinceEpoch(static_cast<int64_t>(timestamp));
  }

  base::Time time;
  if (base::Time::FromUTCString(value.data(), &time)) {
    return TimeDeltaSinceEpoch(time.ToTimeT());
  }

  return std::nullopt;
}

}  // namespace brave_ads
