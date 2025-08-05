/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Replace PasswordManagerSettingsService for Android with the same class as
// desktop is using. This class was used by Brave Android before cr140.
// This change is required to allow Android keep using Password Manager on cr140
// and above.
#define BRAVE_PASSWORD_MANAGER_SETTINGS_SERVICE_FACTORY_CREATE_SERVICE \
  return std::make_unique<                                             \
      password_manager::PasswordManagerSettingsServiceImpl>(           \
      profile->GetPrefs());

#include <chrome/browser/password_manager/password_manager_settings_service_factory.cc>

#undef BRAVE_PASSWORD_MANAGER_SETTINGS_SERVICE_FACTORY_CREATE_SERVICE
