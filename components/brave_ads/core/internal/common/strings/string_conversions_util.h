/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_

#include <string>
#include <vector>

namespace brave_ads {

std::string BoolToString(bool value);

std::vector<float> ConvertStringToVector(const std::string& string,
                                         const std::string& delimiter);
std::string ConvertVectorToString(const std::vector<float>& vector,
                                  const std::string& delimiter);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
