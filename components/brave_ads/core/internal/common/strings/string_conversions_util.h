/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_

#include <string>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"

namespace brave_ads {

std::string BoolToString(bool value);

std::vector<float> ConvertDelimitedStringToVector(const std::string& string,
                                                  const std::string& delimiter);

template <typename T>
std::string ConvertVectorToDelimitedString(const std::vector<T>& vector,
                                           const std::string& delimiter) {
  std::vector<std::string> list;
  list.reserve(vector.size());
  for (const auto& item : vector) {
    list.emplace_back(base::NumberToString(item));
  }

  return base::JoinString(list, delimiter);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
