/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/locale/subdivision_code_util.h"

#include <vector>

#include "base/check_op.h"
#include "base/strings/string_split.h"

namespace ads {
namespace locale {

std::string GetCountryCode(const std::string& code) {
  const std::vector<std::string> components =
      base::SplitString(code, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  DCHECK_EQ(2UL, components.size());

  return components.front();
}

std::string GetSubdivisionCode(const std::string& code) {
  const std::vector<std::string> components =
      base::SplitString(code, "-", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  DCHECK_EQ(2UL, components.size());

  return components.back();
}

}  // namespace locale
}  // namespace ads
