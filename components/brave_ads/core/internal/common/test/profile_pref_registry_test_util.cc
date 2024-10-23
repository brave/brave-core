/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/pref_value_test_info.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_storage_test_util_internal.h"

namespace brave_ads::test {

void RegisterProfilePref(const std::string& path, base::Value default_value) {
  CHECK(!HasProfilePref(path))
      << "Trying to register a previously registered pref: " << path;

  ProfilePref(path).default_value = std::move(default_value);
}

void RegisterProfileBooleanPref(const std::string& path,
                                const bool default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileIntegerPref(const std::string& path,
                                const int default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileDoublePref(const std::string& path,
                               const double default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileStringPref(const std::string& path,
                               const std::string& default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileDictPref(const std::string& path,
                             base::Value::Dict default_value) {
  RegisterProfilePref(path, base::Value(std::move(default_value)));
}

void RegisterProfileListPref(const std::string& path,
                             base::Value::List default_value) {
  RegisterProfilePref(path, base::Value(std::move(default_value)));
}

void RegisterProfileInt64Pref(const std::string& path,
                              const int64_t default_value) {
  RegisterProfilePref(path, base::Int64ToValue(default_value));
}

void RegisterProfileUint64Pref(const std::string& path,
                               const uint64_t default_value) {
  RegisterProfilePref(path, base::Value((base::NumberToString(default_value))));
}

void RegisterProfileTimePref(const std::string& path,
                             const base::Time default_value) {
  RegisterProfilePref(path, base::TimeToValue(default_value));
}

void RegisterProfileTimeDeltaPref(const std::string& path,
                                  const base::TimeDelta default_value) {
  RegisterProfilePref(path, base::TimeDeltaToValue(default_value));
}

}  // namespace brave_ads::test
