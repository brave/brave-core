/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Unlike |ads_client_mock_->Set*Pref| in |UnitTestBase|, |Set*PrefValue| will
// not notify observers.
void SetBooleanPrefValue(const std::string& path, bool value);
void SetIntegerPrefValue(const std::string& path, int value);
void SetDoublePrefValue(const std::string& path, double value);
void SetStringPrefValue(const std::string& path, const std::string& value);
void SetInt64PrefValue(const std::string& path, int64_t value);
void SetUint64PrefValue(const std::string& path, uint64_t value);
void SetDictPrefValue(const std::string& path, base::Value::Dict value = {});
void SetListPrefValue(const std::string& path, base::Value::List value = {});
void SetTimePrefValue(const std::string& path, base::Time value);
void SetLocalStatePrefValue(const std::string& path, const std::string& value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_UTIL_H_
