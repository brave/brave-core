/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_value.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_local_state_pref_storage.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"

namespace brave_ads {

void SetLocalStatePrefValue(const std::string& path, base::Value value) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  LocalStatePref(path).value = std::move(value);
}

void SetDefaultLocalStatePrefValue(const std::string& path,
                                   base::Value default_value) {
  CHECK(HasLocalStatePref(path))
      << "Check failed. Trying to write an unregistered pref: " << path;

  LocalStatePref(path).default_value = std::move(default_value);
}

void SetLocalStateBooleanPrefValue(const std::string& path, const bool value) {
  SetLocalStatePrefValue(path, base::Value(value));
}

void SetLocalStateIntegerPrefValue(const std::string& path, const int value) {
  SetLocalStatePrefValue(path, base::Value(value));
}

void SetLocalStateDoublePrefValue(const std::string& path, const double value) {
  SetLocalStatePrefValue(path, base::Value(value));
}

void SetLocalStateStringPrefValue(const std::string& path,
                                  const std::string& value) {
  SetLocalStatePrefValue(path, base::Value(value));
}

void SetLocalStateInt64PrefValue(const std::string& path, const int64_t value) {
  SetLocalStatePrefValue(path, base::Int64ToValue(value));
}

void SetLocalStateUint64PrefValue(const std::string& path,
                                  const uint64_t value) {
  SetLocalStatePrefValue(path, base::Value(base::NumberToString(value)));
}

void SetLocalStateDictPrefValue(const std::string& path,
                                base::Value::Dict value) {
  SetLocalStatePrefValue(path, base::Value(std::move(value)));
}

void SetLocalStateListPrefValue(const std::string& path,
                                base::Value::List value) {
  SetLocalStatePrefValue(path, base::Value(std::move(value)));
}

void SetLocalStateTimePrefValue(const std::string& path, base::Time value) {
  SetLocalStatePrefValue(path, base::TimeToValue(value));
}

}  // namespace brave_ads
