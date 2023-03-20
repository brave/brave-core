/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace brave_ads {

namespace {

constexpr char kTrue[] = "true";
constexpr char kFalse[] = "false";

}  // namespace

std::string BoolToString(const bool value) {
  return value ? kTrue : kFalse;
}

std::vector<float> ConvertDelimitedStringToVector(
    const std::string& string,
    const std::string& delimiter) {
  const std::vector<std::string> components = base::SplitString(
      string, delimiter, base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  std::vector<float> vector;
  vector.reserve(components.size());
  for (const auto& component : components) {
    double value;
    base::StringToDouble(component, &value);
    vector.push_back(static_cast<float>(value));
  }

  return vector;
}

}  // namespace brave_ads
