/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/country_code_user_data.h"

#include <string>
#include <string_view>

#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "components/variations/pref_names.h"

namespace brave_ads {

namespace {
constexpr std::string_view kCountryCodeKey = "countryCode";
}  // namespace

base::Value::Dict BuildCountryCodeUserData() {
  std::string country_code =
      GetLocalStateStringPref(variations::prefs::kVariationsCountry);
  if (country_code.empty()) {
    // If the variations country code is not set, use the device's current
    // country code as a fallback.
    country_code = CurrentCountryCode();
  }
  return base::Value::Dict().Set(kCountryCodeKey, country_code);
}

}  // namespace brave_ads
