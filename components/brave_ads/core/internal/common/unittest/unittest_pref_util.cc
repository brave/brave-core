/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_current_test_util.h"

namespace brave_ads {

void SetDefaultBooleanPref(const std::string& path, const bool value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = base::NumberToString(static_cast<int>(value));
}

void SetDefaultIntegerPref(const std::string& path, const int value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = base::NumberToString(value);
}

void SetDefaultDoublePref(const std::string& path, const double value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = base::NumberToString(value);
}

void SetDefaultStringPref(const std::string& path, const std::string& value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = value;
}

void SetDefaultInt64Pref(const std::string& path, const int64_t value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = base::NumberToString(value);
}

void SetDefaultUint64Pref(const std::string& path, const uint64_t value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] = base::NumberToString(value);
}

void SetDefaultDictPref(const std::string& path, base::Value::Dict value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  CHECK(base::JSONWriter::Write(value, &Prefs()[uuid]));
}

void SetDefaultListPref(const std::string& path, base::Value::List value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  CHECK(base::JSONWriter::Write(value, &Prefs()[uuid]));
}

void SetDefaultTimePref(const std::string& path, const base::Time value) {
  const std::string uuid = GetUuidForCurrentTestAndValue(path);
  Prefs()[uuid] =
      base::NumberToString(value.ToDeltaSinceWindowsEpoch().InMicroseconds());
}

}  // namespace brave_ads
