/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/condition_matcher_pref_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_pref_util_internal.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace brave_ads {

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
