/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <components/sync/service/sync_auth_manager.cc>

namespace syncer {

// We want to override this in BraveSyncAuthManager, but for the base
// implementation fall back onto the anonymous function.
SyncAccountInfo SyncAuthManager::DetermineAccountToUse(
    const signin::IdentityManager* identity_manager) const {
  return syncer::DetermineAccountToUse(identity_manager);
}

}  // namespace syncer
