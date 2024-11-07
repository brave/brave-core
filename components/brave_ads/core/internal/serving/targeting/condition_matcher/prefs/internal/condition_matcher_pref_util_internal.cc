/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/targeting/condition_matcher/prefs/internal/condition_matcher_pref_util_internal.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/public/prefs/pref_provider_interface.h"

namespace brave_ads {

std::optional<std::string> ToString(const base::Value& value) {
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
               << base::to_underlying(value.type());
}

std::optional<base::Value> MaybeGetRootPrefValue(
    const PrefProviderInterface* const pref_provider,
    const std::string& pref_path) {
  CHECK(pref_provider);

  if (pref_path.starts_with(kVirtualPrefPathPrefix)) {
    return pref_provider->GetVirtualPref(pref_path);
  }

  if (std::optional<base::Value> pref_value =
          pref_provider->GetProfilePref(pref_path)) {
    return pref_value;
  }

  if (std::optional<base::Value> pref_value =
          pref_provider->GetLocalStatePref(pref_path)) {
    return pref_value;
  }

  // Unknown pref path.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetDictPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  if (const base::Value* const value = pref_value.GetDict().Find(key)) {
    return value->Clone();
  }

  // Unknown pref path key.
  return std::nullopt;
}

std::optional<base::Value> MaybeGetListPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  int index;
  if (!base::StringToInt(key, &index)) {
    // Invalid pref path key, because this should be an integer index for the
    // list.
    return std::nullopt;
  }

  const base::Value::List& list = pref_value.GetList();

  if (index < 0 || index >= static_cast<int>(list.size())) {
    // Invalid pref path key, because the list index is out of bounds.
    return std::nullopt;
  }

  return list[index].Clone();
}

std::optional<base::Value> MaybeGetNextPrefValue(const base::Value& pref_value,
                                                 const std::string& key) {
  if (pref_value.is_dict()) {
    return MaybeGetDictPrefValue(pref_value, key);
  }

  if (pref_value.is_list()) {
    return MaybeGetListPrefValue(pref_value, key);
  }

  return std::nullopt;
}

std::optional<base::Value> MaybeGetPrefValue(
    const PrefProviderInterface* const pref_provider,
    const std::string& pref_path) {
  CHECK(pref_provider);

  // Split the `pref_path` into individual keys using '|' as the delimiter.
  const std::vector<std::string> keys = base::SplitString(
      pref_path, "|", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (keys.empty()) {
    // Invalid pref path.
    VLOG(1) << "Invalid condition matcher pref path: " << pref_path;
    return std::nullopt;
  }

  std::optional<base::Value> pref_value;

  for (auto iter = keys.cbegin(); iter != keys.cend(); ++iter) {
    const std::string& key = *iter;

    if (!pref_value) {
      // Attempt to get the root pref value using the current key.
      if (std::optional<base::Value> root_pref_value =
              MaybeGetRootPrefValue(pref_provider, key)) {
        pref_value = std::move(*root_pref_value);
        continue;
      }

      // Unknown pref path key.
      VLOG(1) << "Unknown condition matcher " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    // Attempt to get the next pref value in the path.
    pref_value = MaybeGetNextPrefValue(*pref_value, key);
    if (!pref_value) {
      // Unknown pref path key.
      VLOG(1) << "Unknown condition matcher " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    if (pref_value->is_dict() || pref_value->is_list()) {
      // Continue iterating if the current pref value is a dictionary or list.
      continue;
    }

    if (iter != keys.cend() - 1) {
      // Invalid pref path, because this should be the last pref path key.
      VLOG(1) << "Invalid condition matcher " << key << " key for " << pref_path
              << " pref path";
      return std::nullopt;
    }

    break;
  }

  // Return the last pref path value.
  return pref_value;
}

}  // namespace brave_ads
