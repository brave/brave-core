/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/ads.h"

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/ranges/algorithm.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/geographic/country/supported_country_codes.h"
#include "brave/components/l10n/common/locale_util.h"

namespace ads {

bool IsSupportedLocale(const std::string& locale) {
  const std::string country_code = brave_l10n::GetISOCountryCode(locale);

  return base::ranges::any_of(geographic::GetSupportedCountryCodes(),
                              [&country_code](const auto& schema) {
                                return base::Contains(schema.second,
                                                      country_code);
                              });
}

// static
Ads* Ads::CreateInstance(AdsClient* ads_client) {
  DCHECK(ads_client);
  return new AdsImpl(ads_client);
}

}  // namespace ads
