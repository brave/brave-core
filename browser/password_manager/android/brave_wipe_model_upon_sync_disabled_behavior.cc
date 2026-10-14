/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/password_manager/android/brave_wipe_model_upon_sync_disabled_behavior.h"

#include "base/feature_list.h"
#include "brave/components/brave_sync/features.h"

namespace brave_password_manager {

syncer::WipeModelUponSyncDisabledBehavior GetWipeModelUponSyncDisabledBehavior(
    password_manager::IsAccountStore is_account_store) {
  // Under kBraveAndroidSyncPasswordsInProfileStore the profile store is the
  // synced one and the account store is drained into it by the startup
  // migrator, so account-store credentials have to survive leaving the sync
  // chain. The profile store is never wiped either way.
  const bool wipe_account_store_upon_sync_disabled =
      is_account_store.value() &&
      !base::FeatureList::IsEnabled(
          brave_sync::features::kBraveAndroidSyncPasswordsInProfileStore);
  return wipe_account_store_upon_sync_disabled
             ? syncer::WipeModelUponSyncDisabledBehavior::kAlways
             : syncer::WipeModelUponSyncDisabledBehavior::kNever;
}

}  // namespace brave_password_manager
