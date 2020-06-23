/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/prefs/pref_service.h"

#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "brave/components/sync/driver/brave_sync_stopped_reporter.h"

#define BRAVE_PROFILE_SYNC_SERVICE                                         \
  brave_sync_prefs_change_registrar_.Init(sync_client_->GetPrefService()); \
  brave_sync_prefs_change_registrar_.Add(                                  \
      brave_sync::Prefs::GetSeedPath(),                                    \
      base::Bind(&ProfileSyncService::OnBraveSyncPrefsChanged,             \
                 base::Unretained(this)));                                 \
  brave_sync::Prefs brave_sync_prefs(sync_client_->GetPrefService());      \
  auth_manager_->DeriveSigningKeys(brave_sync_prefs.GetSeed());            \
  if (!brave_sync_prefs.IsSyncV1Migrated()) {                              \
    StopImpl(CLEAR_DATA);                                                  \
    brave_sync_prefs.SetSyncV1Migrated(true);                              \
  }

#define BRAVE_D_PROFILE_SYNC_SERVICE \
  brave_sync_prefs_change_registrar_.RemoveAll();

#define SyncAuthManager BraveSyncAuthManager
#define SyncStoppedReporter BraveSyncStoppedReporter

#include "../../../../../components/sync/driver/profile_sync_service.cc"

#undef BRAVE_PROFILE_SYNC_SERVICE
#undef BRAVE_D_PROFILE_SYNC_SERVICE
#undef SyncAuthManager
#undef SyncStoppedReporter

namespace syncer {
void ProfileSyncService::OnBraveSyncPrefsChanged(const std::string& path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (path == brave_sync::Prefs::GetSeedPath()) {
    brave_sync::Prefs brave_sync_prefs(sync_client_->GetPrefService());
    const std::string seed = brave_sync_prefs.GetSeed();
    if (!seed.empty()) {
      auth_manager_->DeriveSigningKeys(seed);
    } else {
      VLOG(1) << "Brave sync seed cleared";
      auth_manager_->ResetKeys();
    }
  }
}
}  // namespace syncer
