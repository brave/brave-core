/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/locale/country_code_util.h"

#include "base/containers/contains.h"
#include "base/strings/string_util.h"
#include "bat/ads/internal/privacy/locale/country_code_anonymity_set.h"
#include "bat/ads/internal/privacy/locale/other_country_codes.h"

namespace ads::privacy::locale {

bool IsCountryCodeMemberOfAnonymitySet(const std::string& country_code) {
  return base::Contains(kCountryCodeAnonymitySet,
                        base::ToUpperASCII(country_code));
}

bool ShouldClassifyCountryCodeAsOther(const std::string& country_code) {
  return base::Contains(kOtherCountryCodes, base::ToUpperASCII(country_code));
}

}  // namespace ads::privacy::locale
