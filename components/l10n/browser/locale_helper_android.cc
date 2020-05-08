/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/locale_helper_android.h"

#include "base/android/locale_utils.h"

namespace brave_l10n {

std::string LocaleHelperAndroid::GetLocale() const {
  return base::android::GetDefaultLocaleString();
}

LocaleHelperAndroid* LocaleHelperAndroid::GetInstanceImpl() {
  return base::Singleton<LocaleHelperAndroid>::get();
}

LocaleHelper* LocaleHelper::GetInstanceImpl() {
  return LocaleHelperAndroid::GetInstanceImpl();
}

}  // namespace brave_l10n
