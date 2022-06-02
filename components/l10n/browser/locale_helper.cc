/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/l10n/browser/locale_helper.h"

#include "base/logging.h"

namespace brave_l10n {

LocaleHelper* g_locale_helper_for_testing = nullptr;

LocaleHelper::LocaleHelper() = default;
LocaleHelper::~LocaleHelper() = default;

/*static*/
void LocaleHelper::SetForTesting(LocaleHelper* locale_helper) {
  g_locale_helper_for_testing = locale_helper;
}

std::string LocaleHelper::GetLocale() const {
  return kDefaultLocale;
}

LocaleHelper* LocaleHelper::GetInstance() {
  if (g_locale_helper_for_testing) {
    return g_locale_helper_for_testing;
  }

  return GetInstanceImpl();
}

#if !BUILDFLAG(IS_APPLE) && !BUILDFLAG(IS_WIN) && !BUILDFLAG(IS_LINUX) && \
    !BUILDFLAG(IS_ANDROID)  // NOLINT
LocaleHelper* LocaleHelper::GetInstanceImpl() {
  // Return a default locale helper for unsupported platforms
  return base::Singleton<LocaleHelper>::get();
}
#endif

}  // namespace brave_l10n
