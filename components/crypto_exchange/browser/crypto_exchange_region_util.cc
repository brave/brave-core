/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/crypto_exchange/browser/crypto_exchange_region_util.h"

#include "components/prefs/pref_service.h"
#include "components/country_codes/country_codes.h"

namespace crypto_exchange {

bool IsRegionSupported(PrefService* pref_service,
    std::vector<std::string> regions,
    bool allow_list)  {
  bool is_supported = !allow_list;
  const int32_t user_region_id =
      country_codes::GetCountryIDFromPrefs(pref_service);

  for (const auto& region : regions) {
    const int32_t region_id = country_codes::CountryCharsToCountryID(
        region.at(0), region.at(1));
    if (user_region_id == region_id) {
      is_supported = !is_supported;
      break;
    }
  }

  return is_supported;
}

}  // namespace crypto_exchange
