/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_

namespace syncer {
class BraveSyncAuthManager;
}  // namespace syncer

#define RequestAccessToken virtual RequestAccessToken

// Add DetermineAccountToUse method that would hide the function with the same
// signature in the anonymous namespace in the .cc file. Also, add friend for
// our derived class.
#define UpdateSyncAccountIfNecessary                          \
  UpdateSyncAccountIfNecessary_Unused();                      \
  virtual SyncAccountInfo DetermineAccountToUse(              \
      const signin::IdentityManager* identity_manager) const; \
  friend BraveSyncAuthManager;                                \
  bool UpdateSyncAccountIfNecessary

#include <components/sync/service/sync_auth_manager.h>  // IWYU pragma: export

#undef RequestAccessToken
#undef UpdateSyncAccountIfNecessary
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_
