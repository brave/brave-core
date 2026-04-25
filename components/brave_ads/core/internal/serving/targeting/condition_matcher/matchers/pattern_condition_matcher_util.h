/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_PATTERN_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_PATTERN_CONDITION_MATCHER_UTIL_H_

#include <string_view>

// Matches conditions against string prefs using wildcards, for ads that need
// exact, partial, prefix, or suffix matching without requiring full regular
// expression syntax, all evaluated locally with nothing leaving the device.

namespace brave_ads {

// Returns `true` if `value` matches `condition` as a full string. `*` matches
// zero or more characters, `?` matches exactly one character, and `\` escapes
// either literally.
bool MatchPattern(std::string_view value, std::string_view condition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_PATTERN_CONDITION_MATCHER_UTIL_H_
