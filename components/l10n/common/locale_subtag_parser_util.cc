/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_subtag_parser_util.h"

#include <optional>

#include "absl/types/optional.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/l10n/common/locale_subtag_info.h"

namespace brave_l10n {

namespace {

constexpr char kHyphenSeparator = '-';
constexpr char kUnderscoreSeparator = '_';
constexpr char kCodeSetSeparator = '.';
constexpr char kVariantSeparator = '@';

std::string NormalizeLocale(std::string_view locale) {
  // Calculate length of locale excluding charset and variant.
  const std::string::size_type pos = locale.find_first_of(base::StrCat(
      {std::string{kCodeSetSeparator}, std::string{kVariantSeparator}}));

  const size_t length = (pos == std::string::npos) ? locale.length() : pos;

  // Normalize language, script, and country separators.
  std::string normalized_locale{locale};
  for (size_t i = 0; i < length; i++) {
    if (normalized_locale[i] == kHyphenSeparator) {
      normalized_locale[i] = kUnderscoreSeparator;
    }
  }

  return normalized_locale;
}

std::optional<LocaleSubtagInfo>& CachedLocaleSubtag() {
  static base::NoDestructor<std::optional<LocaleSubtagInfo>>
      cached_locale_subtag;
  return *cached_locale_subtag;
}

std::string& LastLocale() {
  static base::NoDestructor<std::string> last_locale;
  return *last_locale;
}

}  // namespace

LocaleSubtagInfo ParseLocaleSubtags(std::string_view locale) {
  if (CachedLocaleSubtag() && LastLocale() == locale) {
    return *CachedLocaleSubtag();
  }

  LastLocale() = locale;

  LocaleSubtagInfo locale_subtag;

  if (locale.empty()) {
    CachedLocaleSubtag() = locale_subtag;
    return locale_subtag;
  }

  std::string str = NormalizeLocale(locale);

  // Parse variant.
  std::string::size_type pos = str.find(kVariantSeparator);
  if (pos != std::string::npos) {
    locale_subtag.variant = std::string(str, pos + 1);
    str = std::string(str, 0, pos);
  }

  // Parse charset.
  pos = str.find(kCodeSetSeparator);
  if (pos != std::string::npos) {
    locale_subtag.charset = std::string(str, pos + 1);
    str = std::string(str, 0, pos);
  }

  // Parse language.
  pos = str.find(kUnderscoreSeparator);
  if (pos == std::string::npos) {
    // No script or country, so return.
    locale_subtag.language = base::ToLowerASCII(str);
    CachedLocaleSubtag() = locale_subtag;
    return locale_subtag;
  }
  locale_subtag.language = base::ToLowerASCII(std::string{str, 0, pos});

  std::string::size_type last_pos = pos + 1;

  // Parse script.
  pos = str.find(kUnderscoreSeparator, last_pos);
  if (pos != std::string::npos) {
    const std::string script_code = std::string(str, last_pos, pos - last_pos);
    if (!script_code.empty()) {
      // Script should be sentence case, i.e. "Script".
      locale_subtag.script = base::ToLowerASCII(script_code);
      locale_subtag.script[0] = base::ToUpperASCII(locale_subtag.script[0]);
    }
    last_pos = pos + 1;
  }

  // Parse country.
  locale_subtag.country = base::ToUpperASCII(std::string{str, last_pos});

  CachedLocaleSubtag() = locale_subtag;
  return locale_subtag;
}

}  // namespace brave_l10n
