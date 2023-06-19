/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_

namespace syncer {
class BraveSyncAuthManager;
}  // namespace syncer

// Header guard to prevent DetermineAccountToUse from getting overriden in it
#include "components/sync/service/sync_auth_manager.h"

#define RequestAccessToken virtual RequestAccessToken
#define DetermineAccountToUse      \
  DetermineAccountToUse_Unused() { \
    return SyncAccountInfo();      \
  }                                \
  friend BraveSyncAuthManager;     \
  virtual SyncAccountInfo DetermineAccountToUse

#include "src/components/sync/service/sync_auth_manager.h"  // IWYU pragma: export

#undef RequestAccessToken
#undef DetermineAccountToUse
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_SERVICE_SYNC_AUTH_MANAGER_H_
