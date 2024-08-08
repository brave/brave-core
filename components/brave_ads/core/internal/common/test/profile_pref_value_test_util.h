/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PROFILE_PREF_VALUE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PROFILE_PREF_VALUE_TEST_UTIL_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads::test {

// Sets preference values but does not notify observers.

void SetProfilePrefValue(const std::string& path, base::Value value);
void SetDefaultProfilePrefValue(const std::string& path, base::Value value);
void SetProfileBooleanPrefValue(const std::string& path, bool value);
void SetProfileIntegerPrefValue(const std::string& path, int value);
void SetProfileDoublePrefValue(const std::string& path, double value);
void SetProfileStringPrefValue(const std::string& path,
                               const std::string& value);
void SetProfileInt64PrefValue(const std::string& path, int64_t value);
void SetProfileUint64PrefValue(const std::string& path, uint64_t value);
void SetProfileDictPrefValue(const std::string& path,
                             base::Value::Dict value = {});
void SetProfileListPrefValue(const std::string& path,
                             base::Value::List value = {});
void SetProfileTimePrefValue(const std::string& path, base::Time value);
void SetProfileTimeDeltaPrefValue(const std::string& path,
                                  base::TimeDelta value);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_PROFILE_PREF_VALUE_TEST_UTIL_H_
