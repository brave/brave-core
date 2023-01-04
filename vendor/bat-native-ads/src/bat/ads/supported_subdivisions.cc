/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/supported_subdivisions.h"

#include "base/strings/string_piece.h"
#include "bat/ads/internal/geographic/subdivision/supported_subdivision_codes.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

SupportedSubdivisions GetSupportedSubdivisions() {
  const std::string country_code = brave_l10n::GetDefaultISOCountryCodeString();

  const auto& subdivision_codes = geographic::GetSupportedSubdivisionCodes();
  const auto iter = subdivision_codes.find(country_code);
  if (iter == subdivision_codes.cend()) {
    return {};
  }

  return iter->second;
}

}  // namespace ads
