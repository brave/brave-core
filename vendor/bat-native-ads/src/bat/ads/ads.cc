/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "base/check.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/locale/supported_country_codes.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

mojom::Environment g_environment = mojom::Environment::kDevelopment;

mojom::SysInfo g_sys_info;

mojom::BuildChannel g_build_channel;

bool g_is_debug = false;

const char g_catalog_schema_resource_id[] = "catalog-schema.json";

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

// static
Ads* Ads::CreateInstance(AdsClient* ads_client) {
  DCHECK(ads_client);
  return new AdsImpl(ads_client);
}

}  // namespace ads
