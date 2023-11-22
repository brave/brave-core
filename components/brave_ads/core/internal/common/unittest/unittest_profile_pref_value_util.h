/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_VALUE_UTIL_H_

#include <string>

#include "base/values.h"

namespace brave_ads {

base::Value GetProfilePrefValue(const std::string& path);

base::Value GetDefaultProfilePrefValue(const std::string& path);

void ClearProfilePrefValue(const std::string& path);

bool HasProfilePrefPathValue(const std::string& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_PROFILE_PREF_VALUE_UTIL_H_
