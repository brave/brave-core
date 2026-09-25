/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/feature_list.h"
#include "brave/components/brave_sync/features.h"
#include "components/password_manager/core/browser/affiliation/affiliated_match_helper.h"
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_store_built_in_backend.h"
#endif  // BUILDFLAG(IS_ANDROID)

#define CreatePasswordStoreBackend CreatePasswordStoreBackend_ChromiumImpl

#include <chrome/browser/password_manager/factories/password_store_backend_factory.cc>

#undef CreatePasswordStoreBackend

std::unique_ptr<password_manager::PasswordStoreBackend>
CreatePasswordStoreBackend(
    password_manager::IsAccountStore is_account_store,
    const base::FilePath& login_db_directory,
    PrefService* prefs,
    os_crypt_async::OSCryptAsync* os_crypt_async,
    affiliations::AffiliationService* affiliation_service) {
#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<password_manager::LoginDatabase> login_db(
      password_manager::CreateLoginDatabase(is_account_store,
                                            login_db_directory, prefs));
  // Under kBraveAndroidSyncPasswordsInProfileStore the profile store is the
  // synced one and the account store is drained into it by the startup
  // migrator, so account-store credentials have to survive leaving the sync
  // chain.
  const bool wipe_account_store_upon_sync_disabled =
      is_account_store &&
      !base::FeatureList::IsEnabled(
          brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  auto behavior = wipe_account_store_upon_sync_disabled
                      ? syncer::WipeModelUponSyncDisabledBehavior::kAlways
                      : syncer::WipeModelUponSyncDisabledBehavior::kNever;
  CHECK(affiliation_service);
  auto affiliated_match_helper =
      std::make_unique<password_manager::AffiliatedMatchHelper>(
          affiliation_service);
  return std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
      std::move(login_db), behavior, prefs, os_crypt_async,
      std::move(affiliated_match_helper));
#else
  return CreatePasswordStoreBackend_ChromiumImpl(
      is_account_store, login_db_directory, prefs, os_crypt_async,
      affiliation_service);
#endif
}
