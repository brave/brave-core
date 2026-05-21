/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_FEATURE_PREF_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_FEATURE_PREF_UTIL_INTERNAL_H_

#include <optional>
#include <string_view>

namespace base {
class Value;
}  // namespace base

// Resolves `[virtual]:feature=<feature_name>` keyword path components into a
// dict whose sub-paths are traversed via the normal `|` separator. Any feature
// name is supported, including future ones not yet known at build time.

namespace brave_ads {

inline constexpr std::string_view kFeatureVirtualPrefKeyword =
    "[virtual]:feature";

// Returns a dict with an `is_overridden` string, either "1" if overridden or
// "0" if not; unknown feature names are treated as not overridden. When a
// field trial is associated the dict also contains a `params` sub-dict of
// param name-value pairs; otherwise `params` is absent and any
// `|params|<param_name>` traversal returns nothing. Returns `std::nullopt` if
// `path_component` does not match `[virtual]:feature=<feature_name>` or the
// feature name is empty.
std::optional<base::Value> MaybeGetFeaturePrefValue(
    std::string_view path_component);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_FEATURE_PREF_UTIL_INTERNAL_H_
