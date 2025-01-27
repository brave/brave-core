/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_service.h"

#define SetUsesSplitStoresAndUPMForLocal \
  SetUsesSplitStoresAndUPMForLocal_ChromiumImpl

#include "src/chrome/browser/password_manager/android/password_manager_android_util.cc"

#undef SetUsesSplitStoresAndUPMForLocal

namespace password_manager_android_util {

namespace {

// On Android passwords can be saved into two stores: kAccountStore and
// kProfileStore. Account store is not synced through Chromium sync and supposed
// to store passwords at Google Account. Profile store is the store which saves
// passwords at profile and passwords are synced as before. Decision which store
// to use is made at PasswordSaveManagerImpl::GetPasswordStoreForSavingImpl.
// Finally, `kPasswordsUseUPMLocalAndSeparateStores` pref is checked.
// The stack:
//        features_util::CanCreateAccountStore
//        features_util::internal::CanAccountStorageBeEnabled
//        features_util::internal::IsUserEligibleForAccountStorage
//        features_util::GetDefaultPasswordStore
//        PasswordFeatureManagerImpl::GetDefaultPasswordStore
//        PasswordSaveManagerImpl::AccountStoreIsDefault
//        PasswordSaveManagerImpl::GetPasswordStoreForSavingImpl
// There are two ways to make passwords be saved at the profile store and be
// synced:
//   1) override PasswordSaveManagerImpl::AccountStoreIsDefault
//   2) set `kPasswordsUseUPMLocalAndSeparateStores` pref to `kOff`
// Choosing option 2.
void ForcePasswordsProfileStore(PrefService* pref_service) {
  pref_service->SetInteger(
      password_manager::prefs::kPasswordsUseUPMLocalAndSeparateStores,
      static_cast<int>(UseUpmLocalAndSeparateStoresState::kOff));
}

}  // namespace

// Prevent deleting passwords local DB during migration.
// Happens at SetUsesSplitStoresAndUPMForLocal =>
//      MaybeDeactivateSplitStoresAndLocalUpm =>
//      MaybeDeleteLoginDataFiles
void SetUsesSplitStoresAndUPMForLocal(
    PrefService* pref_service,
    const base::FilePath& login_db_directory) {
  ForcePasswordsProfileStore(pref_service);
}
}  // namespace password_manager_android_util
