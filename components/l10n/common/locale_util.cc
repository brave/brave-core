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

std::optional<std::string> GetISOScriptCode(std::string_view locale) {
  std::string script = ParseLocaleSubtags(locale).script;
  if (script.empty()) {
    return std::nullopt;
  }

  return script;
}

std::optional<std::string> GetDefaultISOScriptCodeString() {
  return GetISOScriptCode(GetDefaultLocaleString());
}

std::string GetISOCountryCode(std::string_view locale) {
  std::string country = ParseLocaleSubtags(locale).country;
  if (country.empty()) {
    return kFallbackCountryCode;
  }

  return country;
}

std::string GetDefaultISOCountryCodeString() {
  return GetISOCountryCode(GetDefaultLocaleString());
}

std::optional<std::string> GetCharSet(std::string_view locale) {
  std::string charset = ParseLocaleSubtags(locale).charset;
  if (charset.empty()) {
    return std::nullopt;
  }

  return charset;
}

std::optional<std::string> GetDefaultCharSetString() {
  return GetCharSet(GetDefaultLocaleString());
}

std::optional<std::string> GetVariant(std::string_view locale) {
  std::string variant_code = ParseLocaleSubtags(locale).variant;
  if (variant_code.empty()) {
    return std::nullopt;
  }

  return variant_code;
}

std::optional<std::string> GetDefaultVariantString() {
  return GetVariant(GetDefaultLocaleString());
}

}  // namespace brave_l10n
