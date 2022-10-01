/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_util.h"

#include "base/no_destructor.h"
#include "brave/components/l10n/browser/default_locale_util.h"
#include "brave/components/l10n/common/locale_subtag_info.h"
#include "brave/components/l10n/common/locale_subtag_parser_util.h"

namespace brave_l10n {

namespace {

constexpr char kFallbackLocale[] = "en_US";
constexpr char kFallbackLanguageCode[] = "en";
constexpr char kFallbackCountryCode[] = "US";

absl::optional<std::string>& DefaultLocaleForTesting() {
  static base::NoDestructor<absl::optional<std::string>> locale;
  return *locale;
}

}  // namespace

ScopedDefaultLocaleForTesting::ScopedDefaultLocaleForTesting(
    const std::string& locale) {
  last_locale_ = DefaultLocaleForTesting();
  DefaultLocaleForTesting() = locale;
}

ScopedDefaultLocaleForTesting::~ScopedDefaultLocaleForTesting() {
  DefaultLocaleForTesting() = last_locale_;
}

std::string GetDefaultLocaleString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return *locale_for_testing;
  }

  const absl::optional<std::string> default_locale =
      MaybeGetDefaultLocaleString();
  if (!default_locale) {
    return kFallbackLocale;
  }

  return *default_locale;
}

std::string GetISOLanguageCode(const std::string& locale) {
  std::string language = ParseLocaleSubtags(locale).language;
  if (language.empty()) {
    return kFallbackLanguageCode;
  }

  return language;
}

std::string GetDefaultISOLanguageCodeString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return GetISOLanguageCode(*locale_for_testing);
  }

  return GetISOLanguageCode(GetDefaultLocaleString());
}

absl::optional<std::string> GetISOScriptCode(const std::string& locale) {
  std::string script = ParseLocaleSubtags(locale).script;
  if (script.empty()) {
    return absl::nullopt;
  }

  return script;
}

absl::optional<std::string> GetDefaultISOScriptCodeString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return GetISOScriptCode(*locale_for_testing);
  }

  return GetISOScriptCode(GetDefaultLocaleString());
}

std::string GetISOCountryCode(const std::string& locale) {
  std::string country = ParseLocaleSubtags(locale).country;
  if (country.empty()) {
    return kFallbackCountryCode;
  }

  return country;
}

std::string GetDefaultISOCountryCodeString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return GetISOCountryCode(*locale_for_testing);
  }

  return GetISOCountryCode(GetDefaultLocaleString());
}

absl::optional<std::string> GetCharSet(const std::string& locale) {
  std::string charset = ParseLocaleSubtags(locale).charset;
  if (charset.empty()) {
    return absl::nullopt;
  }

  return charset;
}

absl::optional<std::string> GetDefaultCharSetString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return GetCharSet(*locale_for_testing);
  }

  return GetCharSet(GetDefaultLocaleString());
}

absl::optional<std::string> GetVariant(const std::string& locale) {
  std::string variant_code = ParseLocaleSubtags(locale).variant;
  if (variant_code.empty()) {
    return absl::nullopt;
  }

  return variant_code;
}

absl::optional<std::string> GetDefaultVariantString() {
  const absl::optional<std::string> locale_for_testing =
      DefaultLocaleForTesting();
  if (locale_for_testing) {
    return GetVariant(*locale_for_testing);
  }

  return GetVariant(GetDefaultLocaleString());
}

}  // namespace brave_l10n
