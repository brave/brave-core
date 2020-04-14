/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_ANDROID_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_ANDROID_H_

#include <string>

#include "brave/components/brave_ads/browser/locale_helper.h"

namespace brave_ads {

class LocaleHelperAndroid : public LocaleHelper {
 public:
  LocaleHelperAndroid(const LocaleHelperAndroid&) = delete;
  LocaleHelperAndroid& operator=(const LocaleHelperAndroid&) = delete;

  static LocaleHelperAndroid* GetInstanceImpl();

 private:
  friend struct base::DefaultSingletonTraits<LocaleHelperAndroid>;

  LocaleHelperAndroid() = default;
  ~LocaleHelperAndroid() override = default;

  // LocaleHelper impl
  std::string GetLocale() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_LOCALE_HELPER_ANDROID_H_
