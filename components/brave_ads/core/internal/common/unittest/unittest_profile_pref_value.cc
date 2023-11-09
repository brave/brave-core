/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_storage.h"

namespace brave_ads {

void SetProfilePrefValue(const std::string& path, base::Value value) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  ProfilePref(path).value = std::move(value);
}

void SetDefaultProfilePrefValue(const std::string& path,
                                base::Value default_value) {
  CHECK(HasProfilePref(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  ProfilePref(path).default_value = std::move(default_value);
}

void SetProfileBooleanPrefValue(const std::string& path, const bool value) {
  SetProfilePrefValue(path, base::Value(value));
}

void SetProfileIntegerPrefValue(const std::string& path, const int value) {
  SetProfilePrefValue(path, base::Value(value));
}

void SetProfileDoublePrefValue(const std::string& path, const double value) {
  SetProfilePrefValue(path, base::Value(value));
}

void SetProfileStringPrefValue(const std::string& path,
                               const std::string& value) {
  SetProfilePrefValue(path, base::Value(value));
}

void SetProfileInt64PrefValue(const std::string& path, const int64_t value) {
  SetProfilePrefValue(path, base::Int64ToValue(value));
}

void SetProfileUint64PrefValue(const std::string& path, const uint64_t value) {
  SetProfilePrefValue(path, base::Value(base::NumberToString(value)));
}

void SetProfileDictPrefValue(const std::string& path, base::Value::Dict value) {
  SetProfilePrefValue(path, base::Value(std::move(value)));
}

void SetProfileListPrefValue(const std::string& path, base::Value::List value) {
  SetProfilePrefValue(path, base::Value(std::move(value)));
}

void SetProfileTimePrefValue(const std::string& path, base::Time value) {
  SetProfilePrefValue(path, base::TimeToValue(value));
}

}  // namespace brave_ads
