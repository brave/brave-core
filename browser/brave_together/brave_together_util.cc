/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_together/brave_together_util.h"

#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"

namespace brave_together {

bool IsBraveTogetherSupported(Profile* profile)  {
  const std::string us_code = "US";
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  const int32_t us_id = country_codes::CountryCharsToCountryID(
      us_code.at(0), us_code.at(1));
  return user_country_id == us_id;
}

}  // namespace brave_together
