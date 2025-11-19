/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"

#include <string_view>

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/locale/language_code.h"
#include "components/country_codes/country_codes.h"

namespace brave_ads {

namespace {

std::string& MutableCurrentLanguageCode() {
  // ISO 639-1 language code (e.g. "en", "fr", "de").
  static base::NoDestructor<std::string> language_code(base::ToLowerASCII(
      MaybeGetLanguageCodeString().value_or(kDefaultLanguageCode)));
  return *language_code;
}

std::string& MutableCurrentCountryCode() {
  // ISO 3166-1 alpha-2 country code (e.g. "US", "FR", "DE").
  static base::NoDestructor<std::string> country_code([]() {
    const country_codes::CountryId country_id =
        country_codes::GetCurrentCountryID();
    const std::string_view country_code =
        country_id.IsValid() ? country_id.CountryCode() : kDefaultCountryCode;
    return base::ToUpperASCII(country_code);
  }());
  return *country_code;
}

}  // namespace

const std::string& CurrentLanguageCode() {
  return MutableCurrentLanguageCode();
}

std::string& MutableCurrentLanguageCodeForTesting() {
  return MutableCurrentLanguageCode();
}

const std::string& CurrentCountryCode() {
  return MutableCurrentCountryCode();
}

std::string& MutableCurrentCountryCodeForTesting() {
  return MutableCurrentCountryCode();
}

}  // namespace brave_ads
