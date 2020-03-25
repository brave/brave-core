/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/binance_api.h"

#include <string>
#include <memory>

#include "base/environment.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/binance/browser/static_values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"

namespace extensions {
namespace api {

ExtensionFunction::ResponseAction
BinanceGetUserTLDFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  const std::string us_TLD = "us";
  const std::string us_Code = "US";
  const std::string global_TLD = "com";

  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());
  const int32_t us_id = country_codes::CountryCharsToCountryID(
      us_Code.at(0), us_Code.at(1));

  const std::string user_TLD =
      (user_country_id == us_id) ? us_TLD : global_TLD;

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(user_TLD)));
}

ExtensionFunction::ResponseAction
BinanceIsSupportedRegionFunction::Run() {
  Profile* profile = Profile::FromBrowserContext(browser_context());
  if (brave::IsTorProfile(profile)) {
    return RespondNow(Error("Not available in Tor profile"));
  }

  bool is_blacklisted = false;
  const int32_t user_country_id =
      country_codes::GetCountryIDFromPrefs(profile->GetPrefs());

  for (const auto& country : binance::kBinanceBlacklistRegions) {
    const int id = country_codes::CountryCharsToCountryID(
        country.at(0), country.at(1));

    if (id == user_country_id) {
      is_blacklisted = true;
      break;
    }
  }

  return RespondNow(OneArgument(
      std::make_unique<base::Value>(!is_blacklisted)));
}

}  // namespace api
}  // namespace extensions
