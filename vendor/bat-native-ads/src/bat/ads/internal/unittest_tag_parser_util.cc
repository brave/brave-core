/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_tag_parser_util.h"

#include <string>
#include <vector>

#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

namespace {

const char kNowTagValue[] = "now";
const char kDistantPastTagValue[] = "distant_past";
const char kDistantFutureTagValue[] = "distant_future";
const char kFromSecondsTagValue[] = "seconds";
const char kFromMinutesTagValue[] = "minutes";
const char kFromHoursTagValue[] = "hours";
const char kFromDaysTagValue[] = "days";

bool ParseTimeDelta(const std::string& value, base::TimeDelta* time_delta) {
  DCHECK(time_delta);

  const std::vector<std::string> components = base::SplitString(
      value, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  int offset;
  if (!base::StringToInt(components.at(0), &offset)) {
    return false;
  }

  const std::string period = components.at(1);
  if (period == kFromSecondsTagValue) {
    *time_delta = base::TimeDelta::FromSeconds(offset);
  } else if (period == kFromMinutesTagValue) {
    *time_delta = base::TimeDelta::FromMinutes(offset);
  } else if (period == kFromHoursTagValue) {
    *time_delta = base::TimeDelta::FromHours(offset);
  } else if (period == kFromDaysTagValue) {
    *time_delta = base::TimeDelta::FromDays(offset);
  } else {
    return false;
  }

  return true;
}

std::vector<std::string> ParseTagsForText(std::string* text) {
  DCHECK(text);

  re2::StringPiece text_string_piece(*text);
  RE2 r("<(.*)>");

  std::vector<std::string> tags;

  std::string tag;
  while (RE2::FindAndConsume(&text_string_piece, r, &tag)) {
    tag = base::ToLowerASCII(tag);
    tags.push_back(tag);
  }

  return tags;
}

bool ParseTimeTag(std::string* value) {
  DCHECK(value);

  base::Time time;

  if (*value == kNowTagValue) {
    time = Now();
  } else if (*value == kDistantPastTagValue) {
    time = DistantPast();
  } else if (*value == kDistantFutureTagValue) {
    time = DistantFuture();
  } else if (re2::RE2::FullMatch(*value,
                                 "[-+]?[0-9]*.*(seconds|minutes|hours|days)")) {
    base::TimeDelta time_delta;

    if (!ParseTimeDelta(*value, &time_delta)) {
      return false;
    }

    time = Now() + time_delta;
  } else {
    return false;
  }

  *value = base::TimeToISO8601(time);

  return true;
}

void ReplaceTagsForText(std::string* text,
                        const std::vector<std::string>& tags) {
  DCHECK(text);

  for (const auto& tag : tags) {
    const std::vector<std::string> components = base::SplitString(
        tag, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    if (components.size() != 2) {
      FAIL() << "Invalid tag: " << tag;
      return;
    }

    const std::string key = components.at(0);
    std::string value = components.at(1);

    if (key == "time") {
      if (!ParseTimeTag(&value)) {
        FAIL() << "Invalid tag: " << tag;
        return;
      }
    } else {
      FAIL() << "Unknown tag: " << tag;
      return;
    }

    const std::string enclosed_tag = base::StringPrintf("<%s>", tag.c_str());
    const std::string escaped_enclosed_tag = RE2::QuoteMeta(enclosed_tag);

    RE2::Replace(text, escaped_enclosed_tag, value);
  }
}

}  // namespace

void ParseAndReplaceTagsForText(std::string* text) {
  DCHECK(text);

  const std::vector<std::string> tags = ParseTagsForText(text);
  ReplaceTagsForText(text, tags);
}

}  // namespace ads
