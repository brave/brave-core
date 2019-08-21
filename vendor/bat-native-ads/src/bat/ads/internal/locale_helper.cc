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
  std::vector<std::string> locale_components = base::SplitString(locale, ".",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.size() == 0) {
    return ads::kDefaultLanguage;
  }

  auto normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  std::vector<std::string> components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.size() == 0) {
    return ads::kDefaultLanguage;
  }

  auto language_code = components.front();
  return language_code;
}

const std::string Locale::GetRegionCode(const std::string& locale) {
  std::vector<std::string> locale_components = base::SplitString(locale, ".",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.size() == 0) {
    return ads::kDefaultRegion;
  }

  auto normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  std::vector<std::string> components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.size() != 2) {
    return ads::kDefaultRegion;
  }

  auto region_code = components.at(1);
  return region_code;
}

}  // namespace helper
