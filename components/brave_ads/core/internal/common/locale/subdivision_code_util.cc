/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/locale/subdivision_code_util.h"

#include <vector>

#include "base/check_op.h"
#include "base/strings/string_split.h"

namespace brave_ads {

std::string GetCountryCode(const std::string& subdivision) {
  const std::vector<std::string> components = base::SplitString(
      subdivision, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  CHECK_EQ(2U, components.size());

  return components.front();
}

std::string GetSubdivisionCode(const std::string& subdivision) {
  const std::vector<std::string> components = base::SplitString(
      subdivision, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  CHECK_EQ(2U, components.size());

  return components.back();
}

}  // namespace brave_ads
