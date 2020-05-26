/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/browser/brave_together/brave_together_util.h"

#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"

namespace brave_together {

const std::vector<std::string> supported_countries = {
  "CA",  // Canada
  "US"   // United States
};

bool IsBraveTogetherSupported(Profile* profile)  {
  bool is_supported = false;
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : supported_countries) {
    const int32_t country_id = country_codes::CountryCharsToCountryID(
        country.at(0), country.at(1));
    if (user_country_id == country_id) {
      is_supported = true;
      break;
    }
  }

  return is_supported;
}

}  // namespace brave_together
