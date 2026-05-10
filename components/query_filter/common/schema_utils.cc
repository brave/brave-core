// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/common/schema_utils.h"

#include <algorithm>
#include <string_view>

#include "url/gurl.h"

namespace {
inline constexpr char kAllDomainsPattern[] = "*://*/*";
}

namespace query_filter {

absl::flat_hash_set<std::string> GetBlocklistedParamsForSpec(
    const std::vector<schema::Rule>& rules,
    std::string_view spec) {
  absl::flat_hash_set<std::string> result;

  const GURL url = GURL(spec);
  // Go over rules that matches on the |url|.
  for (const auto& rule : rules) {
    // Check if the rule explictly "excludes" the |url| from consideration.
    const auto& exclude_itr =
        std::find_if(rule.exclude.cbegin(), rule.exclude.cend(),
                     [&url](const std::string& str) {
                       return str == kAllDomainsPattern || url.DomainIs(str);
                     });
    // |url| excluded from consideration for the current |rule|
    if (exclude_itr != rule.exclude.cend()) {
      continue;
    }

    // Check if the rule explictly "includes" the |url| for consideration.
    const auto& include_itr =
        std::find_if(rule.include.cbegin(), rule.include.cend(),
                     [&url](const std::string& str) {
                       return str == kAllDomainsPattern || url.DomainIs(str);
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
