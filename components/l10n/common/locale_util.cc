/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_util.h"

#include <optional>

#include "brave/components/l10n/common/default_locale.h"
#include "brave/components/l10n/common/locale_subtag_info.h"
#include "brave/components/l10n/common/locale_subtag_parser_util.h"

namespace brave_l10n {

namespace {

constexpr char kFallbackLanguageCode[] = "en";
constexpr char kFallbackCountryCode[] = "US";

// Returns an uppercase two-letter ISO 3166-1 alpha-2 country code or UN M.49
// code for the given locale, falling back to "US" if the locale does not
// contain a country code. See
// https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2 or
// https://en.wikipedia.org/wiki/UN_M49.
std::string GetISOCountryCode(std::string_view locale) {
  std::string country = ParseLocaleSubtags(locale).country;
  if (country.empty()) {
    return kFallbackCountryCode;
  }

  return country;
}

}  // namespace

const std::string& GetDefaultLocaleString() {
  return DefaultLocaleString();
}

std::string GetISOLanguageCode(std::string_view locale) {
  std::string language = ParseLocaleSubtags(locale).language;
  if (language.empty()) {
    return kFallbackLanguageCode;
  }

  return language;
}

std::string GetDefaultISOLanguageCodeString() {
  return GetISOLanguageCode(GetDefaultLocaleString());
}

std::string GetDefaultISOCountryCodeString() {
  return GetISOCountryCode(GetDefaultLocaleString());
}

}  // namespace brave_l10n
