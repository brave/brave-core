/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_util.h"

#include "base/containers/contains.h"
#include "base/strings/string_piece.h"
#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/supported_subdivision_codes.h"

namespace brave_ads {

bool IsSupportedCountryCodeForSubdivisionTargeting(
    const std::string& country_code) {
  const auto& supported_subdivision_codes = GetSupportedSubdivisionCodes();
  return base::Contains(supported_subdivision_codes, country_code);
}

}  // namespace brave_ads
