/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_

#include <algorithm>
#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace brave_ads {

std::string BoolToString(bool value);

std::vector<float> DelimitedStringToVector(const std::string& string,
                                           std::string_view delimiter);

template <typename T>
std::string VectorToDelimitedString(const std::vector<T>& vector_components,
                                    const std::string_view delimiter) {
  std::vector<std::string> string_components(vector_components.size());

  std::transform(vector_components.cbegin(), vector_components.cend(),
                 string_components.begin(), [](const T& vector_component) {
                   return base::NumberToString(vector_component);
                 });

  return base::JoinString(string_components, delimiter);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
