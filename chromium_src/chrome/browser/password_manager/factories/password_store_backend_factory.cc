/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#include "components/password_manager/core/browser/password_store/login_database.h"
#include "components/password_manager/core/browser/password_store/password_store_built_in_backend.h"
#endif  // BUILDFLAG(IS_ANDROID)

#define CreatePasswordStoreBackend CreatePasswordStoreBackend_ChromiumImpl

#include <chrome/browser/password_manager/factories/password_store_backend_factory.cc>

#undef CreatePasswordStoreBackend

std::unique_ptr<password_manager::PasswordStoreBackend>
CreatePasswordStoreBackend(password_manager::IsAccountStore is_account_store,
                           const base::FilePath& login_db_directory,
                           PrefService* prefs,
                           os_crypt_async::OSCryptAsync* os_crypt_async) {
#if BUILDFLAG(IS_ANDROID)
  std::unique_ptr<password_manager::LoginDatabase> login_db(
      password_manager::CreateLoginDatabase(is_account_store,
                                            login_db_directory, prefs));
  auto behavior = syncer::WipeModelUponSyncDisabledBehavior::kNever;
  return std::make_unique<password_manager::PasswordStoreBuiltInBackend>(
      std::move(login_db), behavior, prefs, os_crypt_async);
#else
  return CreatePasswordStoreBackend_ChromiumImpl(
      is_account_store, login_db_directory, prefs, os_crypt_async);
#endif
}
