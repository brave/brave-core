/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/password_manager/core/browser/password_manager.h"

#define RegisterProfilePrefs RegisterProfilePrefs_ChromiumImpl
#include <components/password_manager/core/browser/password_manager.cc>
#undef RegisterProfilePrefs

namespace password_manager {

void PasswordManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefs_ChromiumImpl(registry);

#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kClearingUndecryptablePasswords, false);
#endif
}

}  // namespace password_manager
