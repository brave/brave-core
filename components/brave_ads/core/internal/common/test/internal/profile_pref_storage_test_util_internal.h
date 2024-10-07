/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_PROFILE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_PROFILE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_

#include <string>

namespace brave_ads::test {

struct PrefValueInfo;

bool FindProfilePref(const std::string& path);

PrefValueInfo& ProfilePref(const std::string& path);

bool HasProfilePref(const std::string& path);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_INTERNAL_PROFILE_PREF_STORAGE_TEST_UTIL_INTERNAL_H_
