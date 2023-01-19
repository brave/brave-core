/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/locale/subdivision_code_util.h"

#include <vector>

#include "base/check_op.h"
#include "base/containers/contains.h"
#include "base/strings/string_split.h"
#include "bat/ads/internal/geographic/subdivision/supported_subdivision_codes.h"

namespace ads::locale {

bool IsSupportedCountryCodeForSubdivisionTargeting(
    const std::string& country_code) {
  const auto& supported_subdivision_codes =
      ::ads::geographic::GetSupportedSubdivisionCodes();
  return base::Contains(supported_subdivision_codes, country_code);
}

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

}  // namespace ads::locale
