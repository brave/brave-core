/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/locale/country_code_util.h"

#include "brave/components/l10n/common/locale_util.h"
#include "bat/ads/internal/locale/anonymous_country_codes.h"
#include "bat/ads/internal/locale/large_anonymity_country_codes.h"

namespace ads {
namespace locale {

bool HasLargeAnonymity(
    const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  const auto iter = kLargeAnonymityCountryCodes.find(country_code);
  if (iter == kLargeAnonymityCountryCodes.end()) {
    return false;
  }

  return true;
}

bool IsAnonymous(
    const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  const auto iter = kAnonymousCountryCodes.find(country_code);
  if (iter == kAnonymousCountryCodes.end()) {
    return false;
  }

  return true;
}

}  // namespace locale
}  // namespace ads
