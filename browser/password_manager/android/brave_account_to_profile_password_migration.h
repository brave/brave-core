/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_
#define BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_

class Profile;

namespace brave_password_manager {

// When kBraveAndroidSyncPasswordsInProfileStore is enabled, moves passwords
// from the account store into the profile store (which becomes the synced
// store under the flag). Copy -> verify present in profile -> delete from
// account, so nothing is deleted before it is confirmed copied. No-op when the
// flag is off or the account store is empty; safe to call on every startup.
void MaybeMigrateAccountPasswordsToProfileStore(Profile* profile);

}  // namespace brave_password_manager

#endif  // BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_
