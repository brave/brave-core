/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_
#define BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"

class Profile;

namespace brave_password_manager {

// When kBraveAndroidSyncPasswordsInProfileStore is enabled, moves passwords
// from the account store into the profile store (which becomes the synced
// store under the flag). Copy -> verify present in profile -> delete from
// account, so nothing is deleted before it is confirmed copied. No-op when the
// flag is off or the account store is empty; safe to call on every startup.
// `on_complete` runs once the migration has finished, including no-op cases.
void MaybeMigrateAccountPasswordsToProfileStore(
    Profile* profile,
    base::OnceClosure on_complete = base::DoNothing());

}  // namespace brave_password_manager

#endif  // BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_ACCOUNT_TO_PROFILE_PASSWORD_MIGRATION_H_
