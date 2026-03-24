/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_CONDITION_MATCHER_PREF_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_CONDITION_MATCHER_PREF_UTIL_H_

#include <optional>
#include <string>
#include <string_view>

namespace base {
class DictValue;
}  // namespace base

// Pref values are exposed as strings for condition matching, regardless of
// whether they are virtual, profile, or local state prefs.

namespace brave_ads {

// Returns the value at `pref_path` as a string, converting bool, integer, and
// double to their string representations. The first path component is the
// registered pref name, which may contain dots (e.g. "foo.bar.baz"); any
// further path components, separated by `|`, traverse into the pref's value.
// Returns `std::nullopt` if the path is malformed or unknown, or a path
// component into a list is neither a keyword path component nor an integer
// index.
std::optional<std::string> MaybeGetPrefValueAsString(
    const base::DictValue& virtual_prefs,
    std::string_view pref_path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_CONDITION_MATCHER_PREF_UTIL_H_
