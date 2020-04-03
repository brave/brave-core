/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "bat/ads/ads.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/static_values.h"

namespace ads {

bool _is_debug = false;
bool _is_testing = false;
Environment _environment = Environment::DEVELOPMENT;

const char _bundle_schema_resource_name[] = "bundle-schema.json";
const char _catalog_schema_resource_name[] = "catalog-schema.json";
const char _catalog_resource_name[] = "catalog.json";
const char _client_resource_name[] = "client.json";

// static
Ads* Ads::CreateInstance(
    AdsClient* ads_client) {
  return new AdsImpl(ads_client);
}

bool Ads::IsSupportedLocale(
    const std::string& locale) {
  const std::string region = GetRegion(locale);

  for (const auto& schema : kSupportedRegionsSchemas) {
    const std::vector<std::string> regions = schema.second;
    const auto iter = std::find(regions.begin(), regions.end(), region);
    if (iter != regions.end()) {
      return true;
    }
  }

  return false;
}

bool Ads::IsNewlySupportedLocale(
    const std::string& locale,
    const int last_schema_version) {
  const std::string region = GetRegion(locale);

  for (const auto& schema : kSupportedRegionsSchemas) {
    const int schema_version = schema.first;
    if (schema_version < last_schema_version) {
      continue;
    }

    const std::vector<std::string> regions = schema.second;
    const auto iter = std::find(regions.begin(), regions.end(), region);
    if (iter != regions.end()) {
      return true;
    }
  }

  return false;
}

std::string Ads::GetRegion(
    const std::string& locale) {
  return helper::Locale::GetRegionCode(locale);
}

}  // namespace ads
