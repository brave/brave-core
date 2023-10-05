/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"

#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref.h"

namespace brave_ads {

void SetBooleanPrefValue(const std::string& path, const bool value) {
  SetPrefValue(path, base::NumberToString(static_cast<int>(value)));
}

void SetIntegerPrefValue(const std::string& path, const int value) {
  SetPrefValue(path, base::NumberToString(static_cast<int>(value)));
}

void SetDoublePrefValue(const std::string& path, const double value) {
  SetPrefValue(path, base::NumberToString(static_cast<int>(value)));
}

void SetStringPrefValue(const std::string& path, const std::string& value) {
  SetPrefValue(path, value);
}

void SetInt64PrefValue(const std::string& path, const int64_t value) {
  SetPrefValue(path, base::NumberToString(static_cast<int>(value)));
}

void SetUint64PrefValue(const std::string& path, const uint64_t value) {
  SetPrefValue(path, base::NumberToString(static_cast<int>(value)));
}

void SetDictPrefValue(const std::string& path, base::Value::Dict value) {
  std::string json;
  CHECK(base::JSONWriter::Write(value, &json));
  SetPrefValue(path, json);
}

void SetListPrefValue(const std::string& path, base::Value::List value) {
  std::string json;
  CHECK(base::JSONWriter::Write(value, &json));
  SetPrefValue(path, json);
}

void SetTimePrefValue(const std::string& path, const base::Time value) {
  SetPrefValue(path, base::NumberToString(
                         value.ToDeltaSinceWindowsEpoch().InMicroseconds()));
}

void SetLocalStatePrefValue(const std::string& path, const std::string& value) {
  SetPrefValue(path, value);
}

}  // namespace brave_ads
