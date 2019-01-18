/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <string>
#include <vector>

#include "locale_helper.h"
#include "string_helper.h"
#include "static_values.h"

namespace helper {

const std::string Locale::GetLanguageCode(const std::string& locale) {
  std::vector<std::string> locale_components = {};
  if (!helper::String::Split(locale, '_', &locale_components)) {
    return ads::kDefaultLanguageCode;
  }

  auto language_code = locale_components.front();
  return language_code;
}

const std::string Locale::GetCountryCode(const std::string& locale) {
  std::vector<std::string> locale_components = {};
  if (!helper::String::Split(locale, '.', &locale_components)) {
    return ads::kDefaultCountryCode;
  }

  auto normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  std::vector<std::string> country_code_components = {};
  helper::String::Split(normalized_locale, '_', &country_code_components);
  if (country_code_components.size() != 2) {
    return ads::kDefaultCountryCode;
  }

  auto country_code = country_code_components.at(1);
  return country_code;
}

}  // namespace helper
