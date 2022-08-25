/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/common/locale_util.h"

#include <algorithm>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/resource/resource_bundle.h"

namespace brave_l10n {

namespace {

const char kDefaultLanguageCode[] = "en";
const char kDefaultCountryCode[] = "US";

}  // namespace

std::string GetLanguageCode(
    const std::string& locale) {
  const std::vector<std::string> locale_components = base::SplitString(locale,
      ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.empty()) {
    return kDefaultLanguageCode;
  }

  std::string normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  const std::vector<std::string> components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.empty()) {
    return kDefaultLanguageCode;
  }

  const std::string language_code = components.front();

  return base::ToLowerASCII(language_code);
}

std::string GetCountryCode(
    const std::string& locale) {
  const std::vector<std::string> locale_components = base::SplitString(locale,
      ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.empty()) {
    return kDefaultCountryCode;
  }

  std::string normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  const std::vector<std::string> components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.size() <= 1) {
    return kDefaultCountryCode;
  }

  const std::string country_code = components.back();

  return base::ToUpperASCII(country_code);
}

std::u16string GetLocalizedResourceUTF16String(int resource_id) {
  std::string value =
      ui::ResourceBundle::GetSharedInstance().LoadLocalizedResourceString(
          resource_id);
  std::u16string output;
  base::UTF8ToUTF16(value.data(), value.size(), &output);
  return output;
}

}  // namespace brave_l10n
