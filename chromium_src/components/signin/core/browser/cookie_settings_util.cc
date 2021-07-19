/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if defined(OS_IOS)
#define SettingsAllowSigninCookies SettingsAllowSigninCookies_ChromiumImpl
#define SettingsDeleteSigninCookiesOnExit \
  SettingsDeleteSigninCookiesOnExit_ChromiumImpl
#endif

#include "../../../../../../components/signin/core/browser/cookie_settings_util.cc"

#if defined(OS_IOS)
#undef SettingsAllowSigninCookies
#undef SettingsDeleteSigninCookiesOnExit

namespace signin {

bool SettingsAllowSigninCookies(
    const content_settings::CookieSettings* cookie_settings) {
  return false;
}

bool SettingsDeleteSigninCookiesOnExit(
    const content_settings::CookieSettings* cookie_settings) {
  return true;
}

}  // namespace signin
#endif
