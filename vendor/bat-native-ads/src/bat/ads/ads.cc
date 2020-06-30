/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <set>

#include "bat/ads/ads.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/supported_regions.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

bool _is_debug = false;
Environment _environment = Environment::DEVELOPMENT;

const char _catalog_schema_resource_name[] = "catalog-schema.json";
const char _catalog_resource_name[] = "catalog.json";
const char _client_resource_name[] = "client.json";

bool IsSupportedLocale(
    const std::string& locale) {
  const std::string region = brave_l10n::GetRegionCode(locale);

  for (const auto& schema : kSupportedRegions) {
    const std::set<std::string> regions = schema.second;
    const auto iter = std::find(regions.begin(), regions.end(), region);
    if (iter != regions.end()) {
      return true;
    }
  }

  return false;
}

bool IsNewlySupportedLocale(
    const std::string& locale,
    const int last_schema_version) {
  const std::string region = brave_l10n::GetRegionCode(locale);

  for (const auto& schema : kSupportedRegions) {
    const int schema_version = schema.first;
    if (schema_version < last_schema_version) {
      continue;
    }

    const std::set<std::string> regions = schema.second;
    const auto iter = std::find(regions.begin(), regions.end(), region);
    if (iter != regions.end()) {
      return true;
    }
  }

  return false;
}

std::string GetRegionCode(
    const std::string& locale) {
  return brave_l10n::GetRegionCode(locale);
}

// static
Ads* Ads::CreateInstance(
    AdsClient* ads_client) {
  return new AdsImpl(ads_client);
}

}  // namespace ads
