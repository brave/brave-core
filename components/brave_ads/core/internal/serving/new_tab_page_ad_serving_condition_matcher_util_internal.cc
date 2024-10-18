/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_condition_matcher_util_internal.h"

#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/types/cxx23_to_underlying.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

namespace {

constexpr char kOperatorMatcherPatternPrefix[] = "[?]:*";
constexpr char kEqualOperatorMatcherPrefix[] = "[=]:";
constexpr char kGreaterThanOperatorMatcherPrefix[] = "[>]:";
constexpr char kGreaterThanOrEqualOperatorMatcherPrefix[] = "[≥]:";

constexpr int32_t kMaxUnixEpochTimestamp = std::numeric_limits<int32_t>::max();

std::optional<base::Value> MaybeGetRootPrefValue(
    const PrefProviderInterface* const pref_provider,
    const std::string& pref_path) {
  CHECK(pref_provider);

  if (pref_path.starts_with(kVirtualPrefPathPrefix)) {
    return pref_provider->GetVirtualPref(pref_path);
  }

  if (std::optional<base::Value> pref_value =
          pref_provider->GetProfilePref(pref_path)) {
    return pref_value;
  }

  if (std::optional<base::Value> pref_value =
          pref_provider->GetLocalStatePref(pref_path)) {
    return pref_value;
  }

  // Unknown pref path.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetDictPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  if (const base::Value* const value = pref_value.GetDict().Find(key)) {
    return value->Clone();
  }

  // Unknown pref path key.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetListPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  int index;
  if (!base::StringToInt(key, &index)) {
    // Invalid pref path key, because this should be an integer index for the
    // list.
    return std::nullopt;
  }

  const base::Value::List& list = pref_value.GetList();

  if (index < 0 || index >= static_cast<int>(list.size())) {
    // Invalid pref path key, because the list index is out of bounds.
    return std::nullopt;
  }

  return list[index].Clone();
}

std::optional<base::Value> MaybeGetNextPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  if (pref_value.is_dict()) {
    return MaybeGetDictPrefValue(pref_value, key);
  }

  if (pref_value.is_list()) {
    return MaybeGetListPrefValue(pref_value, key);
  }

  return std::nullopt;
}

}  // namespace

std::optional<std::string> ToString(const base::Value& value) {
  switch (value.type()) {
    case base::Value::Type::BOOLEAN: {
      return base::NumberToString(static_cast<int>(value.GetBool()));
    }

    case base::Value::Type::INTEGER: {
      return base::NumberToString(value.GetInt());
    }

    case base::Value::Type::DOUBLE: {
      return base::NumberToString(value.GetDouble());
    }

    case base::Value::Type::STRING: {
      return value.GetString();
    }

    case base::Value::Type::NONE:
    case base::Value::Type::BINARY:
    case base::Value::Type::DICT:
    case base::Value::Type::LIST: {
      // Unsupported value type.
      return std::nullopt;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for base::Value::Type: "
                        << base::to_underlying(value.type());
}

std::optional<int> ParseDays(const std::string_view condition) {
  CHECK(base::MatchPattern(condition, kOperatorMatcherPatternPrefix));

  const size_t pos = condition.find(':');
  if (pos == std::string::npos || pos + 1 >= condition.size()) {
    // Malformed operator.
    VLOG(1) << "Malformed SmartNTT days operator for " << condition
            << " condition";
    return std::nullopt;
  }

  int days;
  if (!base::StringToInt(condition.substr(pos + 1), &days)) {
    // Malformed days.
    VLOG(1) << "Malformed SmartNTT days operator for " << condition
            << " condition";
    return std::nullopt;
  }

  if (days < 0) {
    VLOG(1) << "Invalid SmartNTT " << days << " days operator for " << condition
            << " condition";
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

bool MatchOperator(const std::string_view value,
                   const std::string_view condition) {
  if (!base::MatchPattern(condition, kOperatorMatcherPatternPrefix)) {
    // Not an operator.
    return false;
  }

  const std::optional<int> days = ParseDays(condition);
  if (!days) {
    // Invalid days.
    return false;
  }

  int64_t timestamp;
  if (!base::StringToInt64(value, &timestamp)) {
    // Invalid timestamp.
    VLOG(1) << "Invalid SmartNTT " << value << " timestamp operator for "
            << condition << " condition";
    return false;
  }
  const base::TimeDelta time_delta = TimeDeltaSinceEpoch(timestamp);

  if (condition.starts_with(kEqualOperatorMatcherPrefix)) {
    return time_delta.InDays() == days;
  }

  if (condition.starts_with(kGreaterThanOperatorMatcherPrefix)) {
    return time_delta.InDays() > days;
  }

  if (condition.starts_with(kGreaterThanOrEqualOperatorMatcherPrefix)) {
    return time_delta.InDays() >= days;
  }

  // Unknown operator.
  VLOG(1) << "Unknown SmartNTT operator for " << condition << " condition";
  return false;
}

bool MatchRegex(const std::string_view value,
                const std::string_view condition) {
  const re2::RE2 re(condition, re2::RE2::Quiet);
  if (!re.ok()) {
    return false;
  }

  return re2::RE2::PartialMatch(value, re);
}

bool MatchPattern(std::string_view value, std::string_view condition) {
  return base::MatchPattern(value, condition);
}

std::optional<base::Value> MaybeGetPrefValue(
    const PrefProviderInterface* const pref_provider,
    const std::string& pref_path) {
  CHECK(pref_provider);

  // Split the `pref_path` into individual keys using '|' as the delimiter.
  const std::vector<std::string> keys = base::SplitString(
      pref_path, "|", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (keys.empty()) {
    // Invalid pref path.
    VLOG(1) << "Invalid SmartNTT pref path: " << pref_path;
    return std::nullopt;
  }

  std::optional<base::Value> pref_value;

  for (auto iter = keys.cbegin(); iter != keys.cend(); ++iter) {
    const std::string& key = *iter;

    if (!pref_value) {
      // Attempt to get the root pref value using the current key.
      if (std::optional<base::Value> root_pref_value =
              MaybeGetRootPrefValue(pref_provider, key)) {
        pref_value = std::move(*root_pref_value);
        continue;
      }

      // Unknown pref path key.
      VLOG(1) << "Unknown SmartNTT " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    // Attempt to get the next pref value in the path.
    pref_value = MaybeGetNextPrefValue(*pref_value, key);
    if (!pref_value) {
      // Unknown pref path key.
      VLOG(1) << "Unknown SmartNTT " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    if (pref_value->is_dict() || pref_value->is_list()) {
      // Continue iterating if the current pref value is a dictionary or list.
      continue;
    }

    if (iter != keys.cend() - 1) {
      // Invalid pref path, because this should be the last pref path key.
      VLOG(1) << "Invalid SmartNTT " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    break;
  }

  // Return the last pref path value.
  return pref_value;
}

std::optional<std::string> MaybeGetPrefValueAsString(
    const PrefProviderInterface* const pref_provider,
    const std::string& pref_path) {
  CHECK(pref_provider);

  if (const std::optional<base::Value> value =
          MaybeGetPrefValue(pref_provider, pref_path)) {
    return ToString(*value);
  }

  // Unknown pref path.
  return std::nullopt;
}

}  // namespace brave_ads
