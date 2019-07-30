/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/locale_helper_android.h"
#include "base/android/locale_utils.h"
#include "base/strings/string_util.h"
#include "base/strings/string_split.h"
#include <algorithm>
#include <vector>

namespace brave_ads {

std::string LocaleHelperAndroid::GetLocale() const {
  return base::android::GetDefaultLocaleString();
}

LocaleHelperAndroid* LocaleHelperAndroid::GetInstance() {
  return base::Singleton<LocaleHelperAndroid>::get();
}

LocaleHelper* LocaleHelper::GetInstance() {
  return LocaleHelperAndroid::GetInstance();
}

const std::string LocaleHelperAndroid::GetCountryCode(const std::string& locale) {
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


}  // namespace brave_ads
