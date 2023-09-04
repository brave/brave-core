/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_REGISTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_REGISTRY_H_

#include <cstdint>
#include <string>

#include "base/values.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

void RegisterBooleanPref(const std::string& path, bool default_value);
void RegisterIntegerPref(const std::string& path, int default_value);
void RegisterDoublePref(const std::string& path, double default_value);
void RegisterStringPref(const std::string& path,
                        const std::string& default_value);
void RegisterInt64Pref(const std::string& path, int64_t default_value);
void RegisterUint64Pref(const std::string& path, uint64_t default_value);
void RegisterDictPref(const std::string& path,
                      base::Value::Dict default_value = {});
void RegisterListPref(const std::string& path,
                      base::Value::List default_value = {});
void RegisterTimePref(const std::string& path, base::Time default_value);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PREF_REGISTRY_H_
