/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/task/post_task.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/sync/device_info_sync_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace syncer {

BraveProfileSyncService::BraveProfileSyncService(InitParams init_params,
                                                 Profile* profile)
    : ProfileSyncService(std::move(init_params)),
      brave_sync_prefs_(sync_client_->GetPrefService()),
      weak_ptr_factory_(this) {
  brave_sync_prefs_change_registrar_.Init(sync_client_->GetPrefService());
  brave_sync_prefs_change_registrar_.Add(
      brave_sync::Prefs::GetSeedPath(),
      base::Bind(&BraveProfileSyncService::OnBraveSyncPrefsChanged,
                 base::Unretained(this)));
  GetBraveSyncAuthManager()->DeriveSigningKeys(brave_sync_prefs_.GetSeed());
  if (!brave_sync_prefs_.IsSyncV1Migrated()) {
    StopImpl(CLEAR_DATA);
    brave_sync_prefs_.SetSyncV1Migrated(true);
  }

  syncer::DeviceInfoSyncService* device_info_sync_service =
      DeviceInfoSyncServiceFactory::GetForProfile(profile);
  DCHECK(device_info_sync_service);

  local_device_info_provider_ =
      device_info_sync_service->GetLocalDeviceInfoProvider();

  device_info_tracker_ = device_info_sync_service->GetDeviceInfoTracker();
  DCHECK(device_info_tracker_);

  device_info_observer_.Add(device_info_tracker_);
}

BraveProfileSyncService::~BraveProfileSyncService() {
  brave_sync_prefs_change_registrar_.RemoveAll();
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
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed))
    return false;
  if (!brave_sync_prefs_.SetSeed(sync_code))
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

void BraveProfileSyncService::OnDeviceInfoChange() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  const syncer::DeviceInfo* local_device_info =
      local_device_info_provider_->GetLocalDeviceInfo();

  bool found_local_device = false;
  const auto all_devices = device_info_tracker_->GetAllDeviceInfo();
  for (const auto& device : all_devices) {
    if (local_device_info->guid() == device->guid()) {
      found_local_device = true;
      break;
    }
  }

  // When our device was removed from the sync chain by some other device,
  // we don't seee it in devices list, we must reset sync in a proper way
  if (!found_local_device) {
    if (device_info_observer_.IsObserving(device_info_tracker_)) {
      device_info_observer_.Remove(device_info_tracker_);
    }

    // We can't call OnSelfDeviceInfoDeleted directly because we are on
    // remove device execution path, so posting task
    base::OnceClosure empty_cb = base::BindOnce([]() {});
    base::PostTask(
        FROM_HERE, {content::BrowserThread::UI},
        base::BindOnce(&BraveProfileSyncService::OnSelfDeviceInfoDeleted,
                       weak_ptr_factory_.GetWeakPtr(), std::move(empty_cb)));
  }
}

}  // namespace syncer
