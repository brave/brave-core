/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "brave/components/sync/driver/profile_sync_service_delegate.h"
#include "components/prefs/pref_service.h"

namespace syncer {

BraveProfileSyncService::BraveProfileSyncService(
    InitParams init_params,
    std::unique_ptr<ProfileSyncServiceDelegate> profile_service_delegate)
    : ProfileSyncService(std::move(init_params)),
      brave_sync_prefs_(sync_client_->GetPrefService()),
      profile_service_delegate_(std::move(profile_service_delegate)),
      weak_ptr_factory_(this) {
  brave_sync_prefs_change_registrar_.Init(sync_client_->GetPrefService());
  brave_sync_prefs_change_registrar_.Add(
      brave_sync::Prefs::GetSeedPath(),
      base::BindRepeating(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                          base::Unretained(this)));
  GetBraveSyncAuthManager()->DeriveSigningKeys(brave_sync_prefs_.GetSeed());

  profile_service_delegate_->set_profile_sync_service(this);
}

BraveProfileSyncService::~BraveProfileSyncService() {
  brave_sync_prefs_change_registrar_.RemoveAll();
}

void BraveProfileSyncService::Initialize() {
  ProfileSyncService::Initialize();
  if (!brave_sync_prefs_.IsSyncV1Migrated()) {
    StopImpl(CLEAR_DATA);
    brave_sync_prefs_.SetSyncV1Migrated(true);
  }
}

bool BraveProfileSyncService::IsSetupInProgress() const {
  return ProfileSyncService::IsSetupInProgress() &&
         !user_settings_->IsFirstSetupComplete();
}

std::string BraveProfileSyncService::GetOrCreateSyncCode() {
  std::string sync_code = brave_sync_prefs_.GetSeed();
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
  }
  return sync_code;
}

bool BraveProfileSyncService::SetSyncCode(const std::string& sync_code) {
  std::vector<uint8_t> seed;
  std::string sync_code_trimmed;
  base::TrimString(sync_code, " \n\t", &sync_code_trimmed);
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code_trimmed, &seed))
    return false;
  if (!brave_sync_prefs_.SetSeed(sync_code_trimmed))
    return false;
  return true;
}

void BraveProfileSyncService::OnSelfDeviceInfoDeleted(base::OnceClosure cb) {
  // This function will follow normal reset process and set SyncRequested to
  // false
  StopAndClear();
  brave_sync_prefs_.Clear();
  // Sync prefs will be clear in ProfileSyncService::StopImpl
  std::move(cb).Run();
}

BraveSyncAuthManager* BraveProfileSyncService::GetBraveSyncAuthManager() {
  return static_cast<BraveSyncAuthManager*>(auth_manager_.get());
}

void BraveProfileSyncService::OnBraveSyncPrefsChanged(const std::string& path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (path == brave_sync::Prefs::GetSeedPath()) {
    const std::string seed = brave_sync_prefs_.GetSeed();
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

void BraveProfileSyncService::SuspendDeviceObserverForOwnReset() {
  profile_service_delegate_->SuspendDeviceObserverForOwnReset();
}

void BraveProfileSyncService::ResumeDeviceObserver() {
  profile_service_delegate_->ResumeDeviceObserver();
}

}  // namespace syncer
