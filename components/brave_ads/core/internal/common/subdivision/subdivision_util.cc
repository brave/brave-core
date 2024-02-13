/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/subdivision_util.h"

#include <vector>

#include "base/strings/string_split.h"

namespace brave_ads {

namespace {

std::vector<std::string> SplitComponents(const std::string& subdivision) {
  return base::SplitString(subdivision, "-", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

}  // namespace

std::optional<std::string> GetSubdivisionCountryCode(
    const std::string& subdivision) {
  const std::vector<std::string> components = SplitComponents(subdivision);
  if (components.size() != 2) {
    return std::nullopt;
  }

  return components.front();
}

std::optional<std::string> GetSubdivisionCode(const std::string& subdivision) {
  const std::vector<std::string> components = SplitComponents(subdivision);
  if (components.size() != 2) {
    return std::nullopt;
  }

  return components.back();
}

}  // namespace brave_ads
