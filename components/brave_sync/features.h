/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_

#include "base/feature_list.h"

namespace brave_sync {
namespace features {

BASE_DECLARE_FEATURE(kBraveSync);
BASE_DECLARE_FEATURE(kBraveSyncDefaultPasswords);
// Android only: sync passwords via the profile store (like desktop) instead of
// the account store, migrating existing account-store passwords to the profile
// store on startup. Staged rollout; not cleanly reversible once migrated.
BASE_DECLARE_FEATURE(kBraveAndroidSyncPasswordsInProfileStore);

}  // namespace features
}  // namespace brave_sync

#endif  // BRAVE_COMPONENTS_BRAVE_SYNC_FEATURES_H_
