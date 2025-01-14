/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define SetUsesSplitStoresAndUPMForLocal \
  SetUsesSplitStoresAndUPMForLocal_ChromiumImpl

#include "src/chrome/browser/password_manager/android/password_manager_android_util.cc"

#undef SetUsesSplitStoresAndUPMForLocal

namespace password_manager_android_util {

// Prevent deleting passwords local DB during migration.
// Happens at SetUsesSplitStoresAndUPMForLocal =>
//      MaybeDeactivateSplitStoresAndLocalUpm =>
//      MaybeDeleteLoginDataFiles
void SetUsesSplitStoresAndUPMForLocal(
    PrefService* pref_service,
    const base::FilePath& login_db_directory) {}
}  // namespace password_manager_android_util
