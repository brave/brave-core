/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_pref_util_internal.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_time_period_storage_pref_util_internal.h"

namespace brave_ads {

std::optional<std::string> MaybeToString(const base::Value& value) {
  switch (value.type()) {
    case base::Value::Type::BOOLEAN: {
      return base::NumberToString(static_cast<int>(value.GetBool()));
    }

    case base::Value::Type::INTEGER: {
      return base::NumberToString(value.GetInt());
    }

    case base::Value::Type::DOUBLE: {
      return base::NumberToString(value.GetDouble());
    }

    case base::Value::Type::STRING: {
      return value.GetString();
    }

    case base::Value::Type::NONE:
    case base::Value::Type::BINARY:
    case base::Value::Type::DICT:
    case base::Value::Type::LIST: {
      // Unsupported value type.
      return std::nullopt;
    }
  }

  NOTREACHED() << "Unexpected value for base::Value::Type: "
               << std::to_underlying(value.type());
}

std::optional<base::Value> MaybeGetRootPrefValue(
    const base::DictValue& virtual_prefs,
    const std::string& pref_path) {
  if (pref_path.starts_with(kVirtualPrefPathPrefix)) {
    return GetVirtualPref(virtual_prefs, pref_path);
  }

  if (std::optional<base::Value> pref_value = GetProfilePref(pref_path)) {
    return pref_value;
  }

  if (std::optional<base::Value> pref_value = GetLocalStatePref(pref_path)) {
    return pref_value;
  }

  // Unknown pref path.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetDictPrefValue(
    const base::DictValue& dict,
    std::string_view path_component) {
  if (const base::Value* const value = dict.Find(path_component)) {
    return value->Clone();
  }

  // Unknown path component.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetListPrefValue(
    const base::ListValue& list,
    std::string_view path_component) {
  // If `path_component` is a "time_period_storage[=<duration>]" pattern, sum
  // the list values within the given time window.
  if (const auto time_period_storage_cutoff =
          MaybeResolveTimePeriodStorageCutoff(path_component)) {
    return base::Value(
        SumTimePeriodStorageListValues(list, *time_period_storage_cutoff));
  }

  // Otherwise, `path_component` should be an integer index for the list.
  size_t index;
  if (!base::StringToSizeT(path_component, &index)) {
    // Invalid `path_component`, because this should be an integer index for the
    // list.
    return std::nullopt;
  }

  if (index >= list.size()) {
    // Invalid `path_component`, because the list index is out of bounds.
    return std::nullopt;
  }

  return list[index].Clone();
}

std::optional<base::Value> MaybeGetNextPrefValue(
    const base::Value& pref_value,
    std::string_view path_component) {
  if (const auto* dict = pref_value.GetIfDict()) {
    return MaybeGetDictPrefValue(*dict, path_component);
  }

  if (const auto* list = pref_value.GetIfList()) {
    return MaybeGetListPrefValue(*list, path_component);
  }

  return std::nullopt;
}

std::optional<base::Value> MaybeGetPrefValue(
    const base::DictValue& virtual_prefs,
    std::string_view pref_path) {
  // Split the `pref_path` into individual `path_components` using '|' as the
  // delimiter.
  const std::vector<std::string> path_components = base::SplitString(
      pref_path, "|", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (path_components.empty()) {
    // Invalid pref path.
    BLOG(1, "Invalid condition matcher pref path: " << pref_path);
    return std::nullopt;
  }

  std::optional<base::Value> pref_value;

  for (auto iter = path_components.cbegin(); iter != path_components.cend();
       ++iter) {
    const std::string& path_component = *iter;

    if (path_component.empty()) {
      // Invalid pref path, because path components must not be empty.
      BLOG(1, "Invalid condition matcher pref path: " << pref_path);
      return std::nullopt;
    }

    if (!pref_value) {
      // Attempt to get the root pref value using the current `path_component`.
      if (std::optional<base::Value> root_pref_value =
              MaybeGetRootPrefValue(virtual_prefs, path_component)) {
        pref_value = std::move(*root_pref_value);
        continue;
      }

      // Unknown path component.
      return std::nullopt;
    }

    // Attempt to get the next pref value in the path.
    pref_value = MaybeGetNextPrefValue(*pref_value, path_component);
    if (!pref_value) {
      // Unknown path component.
      return std::nullopt;
    }

    if (pref_value->is_dict() || pref_value->is_list()) {
      // Continue iterating if the current pref value is a dictionary or list.
      continue;
    }

    if (iter != path_components.cend() - 1) {
      // Invalid pref path, because this should be the last `path_component`.
      BLOG(1, "Invalid condition matcher `"
                  << path_component << "` path_component for " << pref_path
                  << " pref path");
      return std::nullopt;
    }

    break;
  }

  // Return the last pref path value.
  return pref_value;
}

}  // namespace brave_ads
