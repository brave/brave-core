/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <vector>

#include "bat/ads/internal/locale_helper.h"
#include "bat/ads/internal/static_values.h"

#include "base/strings/string_util.h"
#include "base/strings/string_split.h"

namespace helper {

const std::string Locale::GetLanguageCode(const std::string& locale) {
  std::vector<std::string> locale_components = base::SplitString(locale, "_",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.size() == 0) {
    return ads::kDefaultLanguageCode;
  }

  auto language_code = locale_components.front();
  return language_code;
}

const std::string Locale::GetCountryCode(const std::string& locale) {
  std::vector<std::string> locale_components = base::SplitString(locale, ".",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.size() == 0) {
    return ads::kDefaultCountryCode;
  }

  auto normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  std::vector<std::string> country_code_components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (country_code_components.size() != 2) {
    return ads::kDefaultCountryCode;
  }

  auto country_code = country_code_components.at(1);
  return country_code;
}

}  // namespace helper
