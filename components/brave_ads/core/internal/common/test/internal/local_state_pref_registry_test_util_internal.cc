/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_registry_test_util_internal.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/local_state_pref_storage_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/pref_value_test_info.h"

namespace brave_ads::test {

void RegisterLocalStatePref(const std::string& path,
                            base::Value default_value) {
  CHECK(!HasLocalStatePref(path))
      << "Trying to register a previously registered pref: " << path;

  LocalStatePref(path).default_value = std::move(default_value);
}

void RegisterLocalStateBooleanPref(const std::string& path,
                                   const bool default_value) {
  RegisterLocalStatePref(path, base::Value(default_value));
}

void RegisterLocalStateIntegerPref(const std::string& path,
                                   const int default_value) {
  RegisterLocalStatePref(path, base::Value(default_value));
}

void RegisterLocalStateDoublePref(const std::string& path,
                                  const double default_value) {
  RegisterLocalStatePref(path, base::Value(default_value));
}

void RegisterLocalStateStringPref(const std::string& path,
                                  const std::string& default_value) {
  RegisterLocalStatePref(path, base::Value(default_value));
}

void RegisterLocalStateDictPref(const std::string& path,
                                base::Value::Dict default_value) {
  RegisterLocalStatePref(path, base::Value(std::move(default_value)));
}

void RegisterLocalStateListPref(const std::string& path,
                                base::Value::List default_value) {
  RegisterLocalStatePref(path, base::Value(std::move(default_value)));
}

void RegisterLocalStateInt64Pref(const std::string& path,
                                 const int64_t default_value) {
  RegisterLocalStatePref(path, base::Int64ToValue(default_value));
}

void RegisterLocalStateUint64Pref(const std::string& path,
                                  const uint64_t default_value) {
  RegisterLocalStatePref(path,
                         base::Value((base::NumberToString(default_value))));
}

void RegisterLocalStateTimePref(const std::string& path,
                                const base::Time default_value) {
  RegisterLocalStatePref(path, base::TimeToValue(default_value));
}

void RegisterLocalStateTimeDeltaPref(const std::string& path,
                                     const base::TimeDelta default_value) {
  RegisterLocalStatePref(path, base::TimeDeltaToValue(default_value));
}

}  // namespace brave_ads::test
