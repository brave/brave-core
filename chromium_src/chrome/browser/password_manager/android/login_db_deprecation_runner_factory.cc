/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/password_manager/android/login_db_deprecation_runner_factory.h"

#define BuildServiceInstanceForBrowserContext \
  BuildServiceInstanceForBrowserContext_ChromiumImpl
#define GetForProfile GetForProfile_ChromiumImpl

#include "src/chrome/browser/password_manager/android/login_db_deprecation_runner_factory.cc"

#undef GetForProfile
#undef BuildServiceInstanceForBrowserContext

std::unique_ptr<KeyedService>
LoginDbDeprecationRunnerFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
#if BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  // Upstream uses OS based password manager now, we decided to keep using
  // browser based PM. Once flag is removed, we intend to retain the code (move
  // it to `brave-core` repo). This `if` is required only to avoid the error
  // below.
  return nullptr;
#else
  // Explicitly call Chromium's implementation in `else` block to avoid error:
  // code will never be executed [-Werror,-Wunreachable-code].
  return BuildServiceInstanceForBrowserContext_ChromiumImpl(context);
#endif
}

password_manager::LoginDbDeprecationRunner*
LoginDbDeprecationRunnerFactory::GetForProfile(Profile* profile) {
#if BUILDFLAG(USE_LOGIN_DATABASE_AS_BACKEND)
  // We intend to use LoginDatabase as the backend for password manager, so
  // normally we should get here.
  return nullptr;
#else
  // Explicitly call Chromium's implementation in `else` block to avoid not used
  // error.
  return GetForProfile_ChromiumImpl(profile);
#endif
}
