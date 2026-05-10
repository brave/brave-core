// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/schema_utils.h"

#include <algorithm>
#include <string_view>

#include "components/url_pattern_index/url_pattern.h"
#include "third_party/abseil-cpp/absl/strings/ascii.h"
#include "url/gurl.h"

namespace query_filter {

base::flat_set<std::string> GetBlocklistedParamsForSpec(
    const std::vector<schema::Rule>& rules,
    std::string_view spec) {
  base::flat_set<std::string> result;

  const GURL url = GURL(spec);
  if (!url.is_valid()) {
    return result;
  }
  // UrlPattern handles glob-style wildcard patterns used in rule.include and
  // rule.exclude (e.g "*://*.instagram.com/*"). We can't use RE2 directly
  // because RE2 requires explicit ".*" for wildcards, so the pattern above
  // would need to be rewritten as ".*://.*\\.instagram\\.com/.*" first.
  const url_pattern_index::UrlPattern::UrlInfo url_info(url);

  // A helper method to detect blank empty strings.
  constexpr auto IsBlank = [](std::string_view s) {
    return s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c) {
             return absl::ascii_isspace(c);
           });
  };

  // Go over rules that matches on the |url|.
  for (const auto& rule : rules) {
    // Check if the rule explictly "excludes" the |url| from consideration.
    const auto& exclude_itr = std::find_if(
        rule.exclude.cbegin(), rule.exclude.cend(),
        [&url_info, &IsBlank](const std::string& str) {
          return !IsBlank(str) &&
                 url_pattern_index::UrlPattern(str).MatchesUrl(url_info);
        });
    // |url| excluded from consideration for the current |rule|
    if (exclude_itr != rule.exclude.cend()) {
      continue;
    }

    // Check if the rule explictly "includes" the |url| for consideration.
    const auto& include_itr = std::find_if(
        rule.include.cbegin(), rule.include.cend(),
        [&url_info, &IsBlank](const std::string& str) {
          return !IsBlank(str) &&
                 url_pattern_index::UrlPattern(str).MatchesUrl(url_info);
        });
    if (include_itr != rule.include.cend()) {
      for (const auto& param : rule.params) {
        result.insert(param);
      }
    }
  }
  return result;
}

}  // namespace query_filter
