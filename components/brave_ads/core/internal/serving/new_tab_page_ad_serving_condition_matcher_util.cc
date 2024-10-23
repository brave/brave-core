/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"

#include <optional>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_condition_matcher_util_internal.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace brave_ads {

namespace {

bool MatchCondition(const std::string_view value,
                    const std::string_view condition) {
  return MatchOperator(value, condition) || MatchPattern(value, condition) ||
         MatchRegex(value, condition);
}

}  // namespace

bool MatchConditions(
    const PrefProviderInterface* const pref_provider,
    const NewTabPageAdConditionMatcherMap& condition_matchers) {
  CHECK(pref_provider);

  return base::ranges::all_of(
      condition_matchers, [pref_provider](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;

        const std::string normalized_pref_path = NormalizePrefPath(pref_path);
        if (const std::optional<std::string> value = MaybeGetPrefValueAsString(
                pref_provider, normalized_pref_path)) {
          return MatchCondition(*value, condition);
        }

        // Do not serve the ad due to an unknown preference path or
        // unsupported value type.
        return false;
      });
}

}  // namespace brave_ads
