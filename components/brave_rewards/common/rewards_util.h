/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_UTIL_H_

#include <string_view>

class PrefService;

namespace brave_rewards {

enum class IsSupportedOptions {
  kNone,
  kSkipRegionCheck,
};

bool IsSupported(PrefService* prefs,
                 IsSupportedOptions options = IsSupportedOptions::kNone);

bool IsUnsupportedRegion();

void SetCountryCodeForOFACTesting(int country_id);

bool IsAutoContributeSupportedForCountry(std::string_view country_code);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_REWARDS_UTIL_H_
