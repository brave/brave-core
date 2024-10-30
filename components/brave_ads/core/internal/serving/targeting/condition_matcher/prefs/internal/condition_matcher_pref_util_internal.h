/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_PREF_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_PREF_UTIL_INTERNAL_H_

#include <optional>
#include <string>

#include "base/values.h"

namespace brave_ads {

class PrefProviderInterface;

std::optional<std::string> ToString(const base::Value& value);

std::optional<base::Value> MaybeGetRootPrefValue(
    const PrefProviderInterface* pref_provider,
    const std::string& pref_path);

std::optional<base::Value> MaybeGetDictPrefValue(const base::Value& pref_value,
                                                 const std::string& key);

std::optional<base::Value> MaybeGetListPrefValue(const base::Value& pref_value,
                                                 const std::string& key);

std::optional<base::Value> MaybeGetNextPrefValue(const base::Value& pref_value,
                                                 const std::string& key);

// Get the pref value from the provider for the given path. Handles nested
// dictionaries, lists, and dot-separated keys.
// `base::Value::Find*ByDottedPath` is not used because path keys can contain
// dots. Returns `std::nullopt` if the path is malformed or unknown. Path keys
// should be separated by `|`. Example `list|1` would return the second
// element of a list.
std::optional<base::Value> MaybeGetPrefValue(
    const PrefProviderInterface* pref_provider,
    const std::string& pref_path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_TARGETING_CONDITION_MATCHER_PREFS_INTERNAL_CONDITION_MATCHER_PREF_UTIL_INTERNAL_H_
