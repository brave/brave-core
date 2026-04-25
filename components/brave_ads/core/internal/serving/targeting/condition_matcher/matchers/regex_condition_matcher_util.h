/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_REGEX_CONDITION_MATCHER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_REGEX_CONDITION_MATCHER_UTIL_H_

#include <string_view>

// Matches conditions against string prefs using RE2 regular expressions, for
// ads that need more expressive matching than wildcards allow, such as matching
// a specific locale, all evaluated locally with nothing leaving the device.

namespace brave_ads {

// Returns `true` if `condition` (an RE2 expression) partially matches `value`,
// or `false` if it is not valid.
bool MatchRegex(std::string_view value, std::string_view condition);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_MATCHERS_REGEX_CONDITION_MATCHER_UTIL_H_
