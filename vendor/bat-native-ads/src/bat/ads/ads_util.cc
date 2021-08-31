/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads_util.h"

#include "bat/ads/internal/locale/supported_country_codes.h"
#include "brave/components/l10n/browser/locale_helper.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

namespace {

bool IsSupportedLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  for (const auto& schema : kSupportedCountryCodes) {
    const SupportedCountryCodesSet country_codes = schema.second;
    const auto iter =
        std::find(country_codes.begin(), country_codes.end(), country_code);
    if (iter != country_codes.end()) {
      return true;
    }
  }

  return false;
}

}  // namespace

bool IsSupported() {
  const std::string locale =
      brave_l10n::LocaleHelper::GetInstance()->GetLocale();

  return IsSupportedLocale(locale);
}

}  // namespace ads
