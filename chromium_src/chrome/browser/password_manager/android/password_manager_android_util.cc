/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/files/file_path.h"
#include "chrome/browser/password_manager/android/password_manager_util_bridge_interface.h"
#include "components/prefs/pref_service.h"

#define MaybeDeleteLoginDatabases MaybeDeleteLoginDatabases_ChromiumImpl

#include <chrome/browser/password_manager/android/password_manager_android_util.cc>

#undef MaybeDeleteLoginDatabases

namespace password_manager_android_util {

// Prevent any ability to delete passwords db on Android
void MaybeDeleteLoginDatabases(
    PrefService* pref_service,
    const base::FilePath& login_db_directory,
    std::unique_ptr<PasswordManagerUtilBridgeInterface> util_bridge) {}

}  // namespace password_manager_android_util
