/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/locale_helper_android.h"
#include "base/android/locale_utils.h"

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


}  // namespace brave_ads
