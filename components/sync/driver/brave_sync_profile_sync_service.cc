/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"

#include <utility>

#include "base/logging.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "components/prefs/pref_service.h"

namespace syncer {

BraveProfileSyncService::BraveProfileSyncService(InitParams init_params)
    : ProfileSyncService(std::move(init_params)) {
  brave_sync_prefs_change_registrar_.Init(sync_client_->GetPrefService());
  brave_sync_prefs_change_registrar_.Add(
      brave_sync::Prefs::GetSeedPath(),
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  brave_sync::Prefs brave_sync_prefs(sync_client_->GetPrefService());
  GetBraveSyncAuthManager()->DeriveSigningKeys(brave_sync_prefs.GetSeed());
  if (!brave_sync_prefs.IsSyncV1Migrated()) {
    StopImpl(CLEAR_DATA);
    brave_sync_prefs.SetSyncV1Migrated(true);
  }
}

BraveProfileSyncService::~BraveProfileSyncService() {
  brave_sync_prefs_change_registrar_.RemoveAll();
}

bool BraveProfileSyncService::IsSetupInProgress() const {
  return ProfileSyncService::IsSetupInProgress() &&
         !user_settings_->IsFirstSetupComplete();
}

BraveSyncAuthManager* BraveProfileSyncService::GetBraveSyncAuthManager() {
  return static_cast<BraveSyncAuthManager*>(auth_manager_.get());
}

void BraveProfileSyncService::OnBraveSyncPrefsChanged(const std::string& path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (path == brave_sync::Prefs::GetSeedPath()) {
    brave_sync::Prefs brave_sync_prefs(sync_client_->GetPrefService());
    const std::string seed = brave_sync_prefs.GetSeed();
    if (!seed.empty()) {
      GetBraveSyncAuthManager()->DeriveSigningKeys(seed);
      // Default enabled types: Bookmarks
      syncer::UserSelectableTypeSet selected_types;
      selected_types.Put(UserSelectableType::kBookmarks);
      GetUserSettings()->SetSelectedTypes(false, selected_types);
    } else {
      VLOG(1) << "Brave sync seed cleared";
      GetBraveSyncAuthManager()->ResetKeys();
    }
  }
}
}  // namespace syncer
