/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_REGISTRY_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_REGISTRY_UTIL_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

void RegisterProfilePref(const std::string& path, base::Value default_value);
void RegisterProfileBooleanPref(const std::string& path, bool default_value);
void RegisterProfileIntegerPref(const std::string& path, int default_value);
void RegisterProfileDoublePref(const std::string& path, double default_value);
void RegisterProfileStringPref(const std::string& path,
                               const std::string& default_value);
void RegisterProfileDictPref(const std::string& path,
                             base::Value::Dict default_value = {});
void RegisterProfileListPref(const std::string& path,
                             base::Value::List default_value = {});
void RegisterProfileInt64Pref(const std::string& path, int64_t default_value);
void RegisterProfileUint64Pref(const std::string& path, uint64_t default_value);
void RegisterProfileTimePref(const std::string& path, base::Time default_value);
void RegisterProfileTimeDeltaPref(const std::string& path,
                                  base::TimeDelta default_value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_REGISTRY_UTIL_H_
