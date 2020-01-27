/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/locale_helper.h"

#include <algorithm>
#include <vector>
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"

namespace brave_ads {

const char kDefaultCountryCode[] = "US";
const char kDefaultLocale[] = "en-US";

LocaleHelper* g_locale_helper_for_testing = nullptr;

LocaleHelper::LocaleHelper() = default;

LocaleHelper::~LocaleHelper() = default;

void LocaleHelper::set_for_testing(
    LocaleHelper* locale_helper) {
  g_locale_helper_for_testing = locale_helper;
}

const std::string LocaleHelper::GetLocale() const {
  return kDefaultLocale;
}

const std::string LocaleHelper::GetCountryCode(
    const std::string& locale) {
  std::vector<std::string> locale_components = base::SplitString(locale, ".",
      base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  if (locale_components.size() == 0) {
    return kDefaultCountryCode;
  }

  auto normalized_locale = locale_components.front();
  std::replace(normalized_locale.begin(), normalized_locale.end(), '-', '_');

  std::vector<std::string> components = base::SplitString(
      normalized_locale, "_", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  if (components.size() != 2) {
    return kDefaultCountryCode;
  }

  auto country_code = components.at(1);
  return country_code;
}

LocaleHelper* LocaleHelper::GetInstance() {
  if (g_locale_helper_for_testing) {
    return g_locale_helper_for_testing;
  }

  return GetInstanceImpl();
}

#if !defined(OS_MACOSX) && !defined(OS_WIN) && !defined(OS_LINUX) && !defined(OS_ANDROID)  // NOLINT
LocaleHelper* LocaleHelper::GetInstanceImpl() {
  // Return a default locale helper for unsupported platforms
  return base::Singleton<LocaleHelper>::get();
}
#endif

}  // namespace brave_ads
