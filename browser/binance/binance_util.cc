/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"
#include "static_values.h"

namespace binance {

bool IsBinanceSupported(Profile* profile)  {
  bool isBlacklistedRegion = false;
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : kBinanceBlacklistRegions) {
    const int id = country_codes::CountryCharsToCountryID(
        country.at(0), country.at(1));
    if (id == user_country_id) {
      isBlacklistedRegion = true;
      break;
    }
  }

  return !isBlacklistedRegion;
}

}  // namespace binance
