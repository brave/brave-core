/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_
#define BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_

#include <string>

#include "components/prefs/pref_change_registrar.h"
#include "components/sync/driver/profile_sync_service.h"

namespace syncer {

class BraveSyncAuthManager;

class BraveProfileSyncService : public ProfileSyncService {
 public:
  explicit BraveProfileSyncService(InitParams init_params);
  ~BraveProfileSyncService() override;

  // SyncService implementation
  bool IsSetupInProgress() const override;

 private:
  BraveSyncAuthManager* GetBraveSyncAuthManager();

  void OnBraveSyncPrefsChanged(const std::string& path);

  PrefChangeRegistrar brave_sync_prefs_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileSyncService);
};
}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_
