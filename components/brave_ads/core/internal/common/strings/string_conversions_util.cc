/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace brave_ads {

namespace {

constexpr char kTrue[] = "true";
constexpr char kFalse[] = "false";

}  // namespace

std::string BoolToString(const bool value) {
  return value ? kTrue : kFalse;
}

std::vector<float> ConvertStringToVector(const std::string& string,
                                         const std::string& delimiter) {
  const std::vector<std::string> vector_string = base::SplitString(
      string, delimiter, base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::vector<float> vector;
  vector.reserve(vector_string.size());
  for (const auto& element_string : vector_string) {
    double element;
    base::StringToDouble(element_string, &element);
    vector.push_back(static_cast<float>(element));
  }

  return vector;
}

std::string ConvertVectorToString(const std::vector<float>& vector,
                                  const std::string& delimiter) {
  std::vector<std::string> string_vector;
  string_vector.reserve(vector.size());
  for (const auto& element : vector) {
    string_vector.emplace_back(base::NumberToString(element));
  }

  return base::JoinString(string_vector, delimiter);
}

}  // namespace brave_ads
