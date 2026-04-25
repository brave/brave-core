/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/password_manager/core/browser/password_store/login_database.h"
#endif  // !BUILDFLAG(IS_ANDROID)

namespace password_manager {

namespace {

#if BUILDFLAG(IS_ANDROID)
LoginDatabase::DeletingUndecryptablePasswordsEnabled GetPolicyFromPrefs(
    PrefService* prefs) {
  return LoginDatabase::DeletingUndecryptablePasswordsEnabled(true);
}
#endif  // BUILDFLAG(IS_ANDROID)

}  // namespace

}  // namespace password_manager

#include <components/password_manager/core/browser/password_store_factory_util.cc>
