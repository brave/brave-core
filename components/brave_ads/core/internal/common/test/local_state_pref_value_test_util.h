/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_LOCAL_STATE_PREF_VALUE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_LOCAL_STATE_PREF_VALUE_TEST_UTIL_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads::test {

// Sets preference values but does not notify observers.

void SetLocalStatePrefValue(const std::string& path, base::Value value);
void SetDefaultLocalStatePrefValue(const std::string& path, base::Value value);
void SetLocalStateBooleanPrefValue(const std::string& path, bool value);
void SetLocalStateIntegerPrefValue(const std::string& path, int value);
void SetLocalStateDoublePrefValue(const std::string& path, double value);
void SetLocalStateStringPrefValue(const std::string& path,
                                  const std::string& value);
void SetLocalStateInt64PrefValue(const std::string& path, int64_t value);
void SetLocalStateUint64PrefValue(const std::string& path, uint64_t value);
void SetLocalStateDictPrefValue(const std::string& path,
                                base::Value::Dict value = {});
void SetLocalStateListPrefValue(const std::string& path,
                                base::Value::List value = {});
void SetLocalStateTimePrefValue(const std::string& path, base::Time value);
void SetLocalStateTimeDeltaPrefValue(const std::string& path,
                                     base::TimeDelta value);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_LOCAL_STATE_PREF_VALUE_TEST_UTIL_H_
