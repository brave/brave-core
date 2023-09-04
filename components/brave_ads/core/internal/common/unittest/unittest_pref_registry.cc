/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_registry_util.h"

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_info.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_registry.h"

namespace brave_ads {

namespace {

void RegisterPref(const std::string& path, const std::string& default_value) {
  CHECK(!HasRegisteredPrefPath(path))
      << "Trying to register a previously registered pref: " << path;

  Pref(path).default_value = default_value;
}

}  // namespace

void RegisterBooleanPref(const std::string& path, const bool default_value) {
  RegisterPref(path, base::NumberToString(static_cast<int>(default_value)));
}

void RegisterIntegerPref(const std::string& path, const int default_value) {
  RegisterPref(path, base::NumberToString(static_cast<int>(default_value)));
}

void RegisterDoublePref(const std::string& path, const double default_value) {
  RegisterPref(path, base::NumberToString(static_cast<int>(default_value)));
}

void RegisterStringPref(const std::string& path,
                        const std::string& default_value) {
  RegisterPref(path, default_value);
}

void RegisterInt64Pref(const std::string& path, const int64_t default_value) {
  RegisterPref(path, base::NumberToString(static_cast<int>(default_value)));
}

void RegisterUint64Pref(const std::string& path, const uint64_t default_value) {
  RegisterPref(path, base::NumberToString(static_cast<int>(default_value)));
}

void RegisterDictPref(const std::string& path,
                      base::Value::Dict default_value) {
  std::string json;
  CHECK(base::JSONWriter::Write(default_value, &json));
  RegisterPref(path, json);
}

void RegisterListPref(const std::string& path,
                      base::Value::List default_value) {
  std::string json;
  CHECK(base::JSONWriter::Write(default_value, &json));
  RegisterPref(path, json);
}

void RegisterTimePref(const std::string& path, const base::Time default_value) {
  RegisterPref(path,
               base::NumberToString(
                   default_value.ToDeltaSinceWindowsEpoch().InMicroseconds()));
}

}  // namespace brave_ads
