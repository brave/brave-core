/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include <algorithm>

#include "base/check.h"
#include "base/no_destructor.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/geographic/country/supported_country_codes.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

mojom::SysInfo& SysInfo() {
  static base::NoDestructor<mojom::SysInfo> sys_info;
  return *sys_info;
}

mojom::Environment g_environment = mojom::Environment::kStaging;

mojom::BuildChannel& BuildChannel() {
  static base::NoDestructor<mojom::BuildChannel> build_channel;
  return *build_channel;
}

bool g_is_debug = false;

const char g_catalog_json_schema_data_resource_name[] = "catalog-schema.json";

bool IsSupportedLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetCountryCode(locale);

  for (const auto& schema : geographic::kSupportedCountryCodes) {
    const geographic::SupportedCountryCodeSet country_codes = schema.second;
    const auto iter =
        std::find(country_codes.cbegin(), country_codes.cend(), country_code);
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
