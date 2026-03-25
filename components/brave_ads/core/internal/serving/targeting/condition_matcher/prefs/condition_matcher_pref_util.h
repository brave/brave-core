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
// double to their string representations. `pref_path` uses `|` as the path
// delimiter because both registered pref names and dict keys within pref values
// may contain dots. For example, "p3a.logs_constellation_prep" is a pref name
// and P3A histogram names such as "Brave.Core.BookmarkCount" appear as dict
// keys inside that pref. Using `.` as a delimiter would be ambiguous so
// `base::Value::Find*ByDottedPath` is not used as it would incorrectly split on
// those dots. Subsequent `|` separated segments traverse into dict or list
// typed pref values. Returns `std::nullopt` if the path is malformed or
// unknown, or a path component into a list is neither a keyword path component
// nor an integer index.
std::optional<std::string> MaybeGetPrefValueAsString(
    const base::DictValue& virtual_prefs,
    std::string_view pref_path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_CONDITION_MATCHER_PREF_UTIL_H_
