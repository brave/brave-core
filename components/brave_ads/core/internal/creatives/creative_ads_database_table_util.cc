/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table_util.h"

#include <vector>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace brave_ads::database::table {

std::string ConditionMatchersToString(
    const ConditionMatcherMap& condition_matchers) {
  std::vector<std::string> condition_matchers_as_string;
  condition_matchers_as_string.reserve(condition_matchers.size());

  for (const auto& [pref_name, condition] : condition_matchers) {
    // We base64 encode the `pref_name` and `condition` to avoid any issues with
    // pref paths and conditions that contain either `|` or `;`.

    const std::string condition_matcher = base::StrCat(
        {base::Base64Encode(pref_name), "|", base::Base64Encode(condition)});
    condition_matchers_as_string.push_back(condition_matcher);
  }

  return base::JoinString(condition_matchers_as_string, ";");
}

ConditionMatcherMap StringToConditionMatchers(const std::string& value) {
  const std::vector<std::string> condition_matchers_as_string =
      base::SplitString(value, ";", base::TRIM_WHITESPACE,
                        base::SPLIT_WANT_NONEMPTY);

  ConditionMatcherMap condition_matchers;
  for (const auto& condition_matcher_as_string : condition_matchers_as_string) {
    const std::vector<std::string> condition_matcher =
        base::SplitString(condition_matcher_as_string, "|",
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (condition_matcher.size() != 2) {
      // Malfomed condition matcher.
      continue;
    }

    std::string pref_path;
    if (!base::Base64Decode(condition_matcher[0], &pref_path)) {
      // Malfomed condition matcher.
      continue;
    }

    std::string condition;
    if (!base::Base64Decode(condition_matcher[1], &condition)) {
      // Malfomed condition matcher.
      continue;
    }

    condition_matchers.emplace(pref_path, condition);
  }

  return condition_matchers;
}

}  // namespace brave_ads::database::table
