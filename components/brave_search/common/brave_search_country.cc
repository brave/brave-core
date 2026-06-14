// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_search/common/brave_search_country.h"

#include <string>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "components/prefs/pref_service.h"

namespace {

// Pref path for the country ID at install time.
// This is the standard Chromium pref that stores the detected country
// as an integer encoding of two ASCII characters.
constexpr char kCountryIdAtInstall[] = "countryid_at_install";

// Pref path for the user-configured Brave Search country override.
constexpr char kBraveSearchCountryPref[] = "brave.search.country";

// Validates that a country code string is a valid ISO 3166-1 alpha-2 code.
bool IsValidCountryCode(const std::string& code) {
  if (code.length() != 2) {
    return false;
  }
  // Must be two uppercase ASCII letters.
  return base::IsAsciiUpper(code[0]) && base::IsAsciiUpper(code[1]);
}

// Extracts a two-letter country code from the OS locale string.
// Locale format is typically "en-US", "af-ZA", "de-DE", etc.
std::string GetCountryFromLocale() {
  std::string locale = base::i18n::GetConfiguredLocale();
  // Look for the country part after the hyphen or underscore.
  size_t separator = locale.find('-');
  if (separator == std::string::npos) {
    separator = locale.find('_');
  }
  if (separator != std::string::npos && locale.length() >= separator + 3) {
    std::string country = locale.substr(separator + 1, 2);
    std::string upper_country = base::ToUpperASCII(country);
    if (IsValidCountryCode(upper_country)) {
      return upper_country;
    }
  }
  return std::string();
}

}  // namespace

namespace brave_search {

std::string CountryIdToCountryCode(int country_id) {
  if (country_id <= 0) {
    return std::string();
  }

  // The country ID is encoded as two ASCII characters packed into an integer:
  // country_id = first_char * 256 + second_char
  // For example: 'U'(85) * 256 + 'S'(83) = 21843 => "US"
  //              'Z'(90) * 256 + 'A'(65) = 23105 => "ZA"
  char first = static_cast<char>((country_id >> 8) & 0xFF);
  char second = static_cast<char>(country_id & 0xFF);

  std::string code;
  code += static_cast<char>(base::ToUpperASCII(first));
  code += static_cast<char>(base::ToUpperASCII(second));

  if (!IsValidCountryCode(code)) {
    VLOG(1) << "Invalid country code derived from countryid_at_install: "
            << country_id;
    return std::string();
  }

  return code;
}

std::string GetBraveSearchCountryCode(PrefService* prefs) {
  if (!prefs) {
    return GetCountryFromLocale();
  }

  // Priority 1: User-configured Brave Search country preference.
  if (prefs->HasPrefPath(kBraveSearchCountryPref)) {
    std::string user_country = prefs->GetString(kBraveSearchCountryPref);
    if (IsValidCountryCode(user_country)) {
      return user_country;
    }
  }

  // Priority 2: Country ID at install time.
  if (prefs->HasPrefPath(kCountryIdAtInstall)) {
    int country_id = prefs->GetInteger(kCountryIdAtInstall);
    std::string code = CountryIdToCountryCode(country_id);
    if (!code.empty()) {
      return code;
    }
  }

  // Priority 3: OS locale fallback.
  return GetCountryFromLocale();
}

}  // namespace brave_search
