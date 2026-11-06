/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_WIPE_MODEL_UPON_SYNC_DISABLED_BEHAVIOR_H_
#define BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_WIPE_MODEL_UPON_SYNC_DISABLED_BEHAVIOR_H_

#include "components/password_manager/core/browser/password_store/password_store_interface.h"
#include "components/sync/model/wipe_model_upon_sync_disabled_behavior.h"

namespace brave_password_manager {

// The behavior a password store backend for `is_account_store` has to be
// created with on Android, see CreatePasswordStoreBackend.
syncer::WipeModelUponSyncDisabledBehavior GetWipeModelUponSyncDisabledBehavior(
    password_manager::IsAccountStore is_account_store);

}  // namespace brave_password_manager

#endif  // BRAVE_BROWSER_PASSWORD_MANAGER_ANDROID_BRAVE_WIPE_MODEL_UPON_SYNC_DISABLED_BEHAVIOR_H_
