/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/profile_pref_registry_test_util.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/profile_pref_storage_test_util_internal.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"

namespace brave_ads::test {

void RegisterProfilePref(const std::string& path, base::Value default_value) {
  PrefRegistrySimple* const registry =
      GetProfilePrefServiceForTesting().registry();

  switch (default_value.type()) {
    case base::Value::Type::BOOLEAN:
      return registry->RegisterBooleanPref(path, default_value.GetBool());
    case base::Value::Type::INTEGER:
      return registry->RegisterIntegerPref(path, default_value.GetInt());
    case base::Value::Type::DOUBLE:
      return registry->RegisterDoublePref(path, default_value.GetDouble());
    case base::Value::Type::STRING:
      return registry->RegisterStringPref(path, default_value.GetString());
    case base::Value::Type::DICT:
      return registry->RegisterDictionaryPref(
          path, std::move(default_value).TakeDict());
    case base::Value::Type::LIST:
      return registry->RegisterListPref(path,
                                        std::move(default_value).TakeList());
    case base::Value::Type::NONE:
    case base::Value::Type::BINARY:
      break;
  }

  NOTREACHED() << "Unsupported pref value type for: " << path;
}

void RegisterProfileBooleanPref(const std::string& path, bool default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileIntegerPref(const std::string& path, int default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileDoublePref(const std::string& path, double default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileStringPref(const std::string& path,
                               const std::string& default_value) {
  RegisterProfilePref(path, base::Value(default_value));
}

void RegisterProfileDictPref(const std::string& path,
                             base::DictValue default_value) {
  RegisterProfilePref(path, base::Value(std::move(default_value)));
}

void RegisterProfileListPref(const std::string& path,
                             base::ListValue default_value) {
  RegisterProfilePref(path, base::Value(std::move(default_value)));
}

void RegisterProfileInt64Pref(const std::string& path, int64_t default_value) {
  RegisterProfilePref(path, base::Int64ToValue(default_value));
}

void RegisterProfileUint64Pref(const std::string& path,
                               uint64_t default_value) {
  RegisterProfilePref(path, base::Value((base::NumberToString(default_value))));
}

void RegisterProfileTimePref(const std::string& path,
                             base::Time default_value) {
  RegisterProfilePref(path, base::TimeToValue(default_value));
}

void RegisterProfileTimeDeltaPref(const std::string& path,
                                  base::TimeDelta default_value) {
  RegisterProfilePref(path, base::TimeDeltaToValue(default_value));
}

}  // namespace brave_ads::test
