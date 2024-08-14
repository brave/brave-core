/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/tag_parser_test_util_internal.h"

#include <optional>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/i18n/time_formatting.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads::test {

namespace {

constexpr char kTimeTagKey[] = "time";
constexpr char kNowTimeTagValue[] = "now";
constexpr char kDistantPastTimeTagValue[] = "distant_past";
constexpr char kDistantFutureTimeTagValue[] = "distant_future";
constexpr char kSecondsDeltaTimeTagValue[] = "seconds";
constexpr char kMinutesDeltaTimeTagValue[] = "minutes";
constexpr char kHoursDeltaTimeTagValue[] = "hours";
constexpr char kDaysDeltaTimeTagValue[] = "days";

std::optional<base::TimeDelta> ParseTimeDelta(const std::string& value) {
  const std::vector<std::string> components = base::SplitString(
      value, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  CHECK_EQ(2U, components.size()) << "Invalid time tag: " << value;

  int n;
  if (!base::StringToInt(components.at(0), &n)) {
    return std::nullopt;
  }

  const std::string& period = components.at(1);
  if (period == kSecondsDeltaTimeTagValue) {
    return base::Seconds(n);
  }

  if (period == kMinutesDeltaTimeTagValue) {
    return base::Minutes(n);
  }

  if (period == kHoursDeltaTimeTagValue) {
    return base::Hours(n);
  }

  if (period == kDaysDeltaTimeTagValue) {
    return base::Days(n);
  }

  return std::nullopt;
}

std::optional<std::string> ParseTimeTagValue(const std::string& value) {
  if (value == kNowTimeTagValue) {
    return NowAsIso8601();
  }

  if (value == kDistantPastTimeTagValue) {
    return DistantPastAsIso8601();
  }

  if (value == kDistantFutureTimeTagValue) {
    return DistantFutureAsIso8601();
  }

  if (re2::RE2::FullMatch(value, "[-+]?[0-9]*.*(seconds|minutes|hours|days)")) {
    const std::optional<base::TimeDelta> time_delta = ParseTimeDelta(value);
    CHECK(time_delta) << "Invalid time tag value: " << value;
    return base::TimeFormatAsIso8601(Now() + *time_delta);
  }

  return std::nullopt;
}

std::vector<std::string> ParseTagsForText(const std::string& text) {
  std::string_view text_string_piece(text);
  const RE2 r("<(.*)>");

  std::vector<std::string> tags;

  std::string tag;
  while (RE2::FindAndConsume(&text_string_piece, r, &tag)) {
    tags.push_back(base::ToLowerASCII(tag));
  }

  return tags;
}

void ReplaceTagsForText(const std::vector<std::string>& tags,
                        std::string& text) {
  for (const auto& tag : tags) {
    const std::vector<std::string> components = base::SplitString(
        tag, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    CHECK_EQ(2U, components.size()) << "Invalid tag: " << tag;

    const std::string& key = components.at(0);
    std::string value = components.at(1);

    if (key == kTimeTagKey) {
      const std::optional<std::string> time_tag_value =
          ParseTimeTagValue(value);
      CHECK(time_tag_value) << "Invalid time tag value: " << value;
      value = *time_tag_value;
    } else {
      NOTREACHED_NORETURN() << "Unsupported tag: " << tag;
    }

    const std::string enclosed_tag =
        base::ReplaceStringPlaceholders("<$1>", {tag}, nullptr);
    const std::string escaped_enclosed_tag = RE2::QuoteMeta(enclosed_tag);
    RE2::Replace(&text, escaped_enclosed_tag, value);
  }
}

}  // namespace

void ParseAndReplaceTags(std::string& text) {
  ReplaceTagsForText(ParseTagsForText(text), text);
}

}  // namespace brave_ads::test
