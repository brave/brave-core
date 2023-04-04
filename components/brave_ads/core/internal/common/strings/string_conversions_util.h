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

std::vector<float> DelimitedStringToVector(const std::string& string,
                                           base::StringPiece delimiter);

template <typename T>
std::string VectorToDelimitedString(const std::vector<T>& vector_components,
                                    const base::StringPiece delimiter) {
  std::vector<std::string> string_components;
  string_components.reserve(vector_components.size());
  for (const auto& vector_component : vector_components) {
    string_components.emplace_back(base::NumberToString(vector_component));
  }

  return base::JoinString(string_components, delimiter);
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_CONVERSIONS_UTIL_H_
