/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/locale/supported_country_codes.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

Environment _environment = Environment::DEVELOPMENT;

BuildChannel _build_channel;

bool _is_debug = false;

const char _catalog_schema_resource_id[] = "catalog-schema.json";

bool IsSupportedLocale(
    const std::string& locale) {
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

bool IsNewlySupportedLocale(
    const std::string& locale,
    const int last_schema_version) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  for (const auto& schema : kSupportedCountryCodes) {
    const int schema_version = schema.first;
    if (schema_version < last_schema_version) {
      continue;
    }

    const SupportedCountryCodesSet country_codes = schema.second;
    const auto iter =
        std::find(country_codes.begin(), country_codes.end(), country_code);
    if (iter != country_codes.end()) {
      return true;
    }
  }

  return false;
}

// static
Ads* Ads::CreateInstance(
    AdsClient* ads_client) {
  return new AdsImpl(ads_client);
}

}  // namespace ads
