/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"

#include <optional>

#include "base/ranges/algorithm.h"
#include "base/strings/pattern.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_condition_matcher_util_internal.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace brave_ads {

bool MatchConditions(const PrefProviderInterface* pref_provider,
                     const NewTabPageAdConditionMatchers& condition_matchers) {
  CHECK(pref_provider);

  return base::ranges::all_of(
      condition_matchers, [pref_provider](const auto& condition_matcher) {
        const auto& [pref_path, condition] = condition_matcher;

        const std::optional<std::string> value =
            MaybeGetPrefValueAsString(pref_provider, pref_path);
        if (!value) {
          // Do not serve the ad due to an unknown preference path or
          // unsupported value type.
          return false;
        }

        return MatchOperator(*value, condition) ||
               base::MatchPattern(*value, condition) ||
               MatchRegex(*value, condition);
      });
}

}  // namespace brave_ads
