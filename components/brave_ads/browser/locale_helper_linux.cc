/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdlib.h>

#include "brave/components/brave_ads/browser/locale_helper_linux.h"

namespace brave_ads {

LocaleHelperLinux::LocaleHelperLinux() = default;

LocaleHelperLinux::~LocaleHelperLinux() = default;

std::string LocaleHelperLinux::GetLocale() const {
  char const *language = nullptr;

  if (!language || !*language) {
    language = std::getenv("LC_CTYPE");
  }

  if (!language || !*language) {
    language = std::getenv("LC_ALL");
  }

  if (!language || !*language) {
    language = std::getenv("LANG");
  }

  if (!language || !*language) {
    language = kDefaultLocale;
  }

  return std::string(language);
}

LocaleHelperLinux* LocaleHelperLinux::GetInstance() {
  return base::Singleton<LocaleHelperLinux>::get();
}

LocaleHelper* LocaleHelper::GetInstance() {
  return LocaleHelperLinux::GetInstance();
}

}  // namespace brave_ads
