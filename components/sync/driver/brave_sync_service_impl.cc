/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/driver/brave_sync_service_impl.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/driver/brave_sync_auth_manager.h"
#include "brave/components/sync/driver/sync_service_impl_delegate.h"
#include "components/prefs/pref_service.h"

namespace syncer {

BraveSyncServiceImpl::BraveSyncServiceImpl(
    InitParams init_params,
    std::unique_ptr<SyncServiceImplDelegate> sync_service_impl_delegate)
    : SyncServiceImpl(std::move(init_params)),
      brave_sync_prefs_(sync_client_->GetPrefService()),
      sync_service_impl_delegate_(std::move(sync_service_impl_delegate)),
      weak_ptr_factory_(this) {
  brave_sync_prefs_change_registrar_.Init(sync_client_->GetPrefService());
  brave_sync_prefs_change_registrar_.Add(
      brave_sync::Prefs::GetSeedPath(),
      base::BindRepeating(&BraveSyncServiceImpl::OnBraveSyncPrefsChanged,
                          base::Unretained(this)));

  bool failed_to_decrypt = false;
  GetBraveSyncAuthManager()->DeriveSigningKeys(
      brave_sync_prefs_.GetSeed(&failed_to_decrypt));
  DCHECK(!failed_to_decrypt);

  sync_service_impl_delegate_->set_profile_sync_service(this);
}

BraveSyncServiceImpl::~BraveSyncServiceImpl() {
  brave_sync_prefs_change_registrar_.RemoveAll();
}

void BraveSyncServiceImpl::Initialize() {
  SyncServiceImpl::Initialize();
  if (!brave_sync_prefs_.IsSyncV1Migrated()) {
    StopAndClear();
    brave_sync_prefs_.SetSyncV1Migrated(true);
  }

  // P3A ping for those who have sync disabled
  if (!user_settings_->IsFirstSetupComplete()) {
    base::UmaHistogramExactLinear("Brave.Sync.Status.2", 0, 3);
  }
}

bool BraveSyncServiceImpl::IsSetupInProgress() const {
  return SyncServiceImpl::IsSetupInProgress() &&
         !user_settings_->IsFirstSetupComplete();
}

void BraveSyncServiceImpl::StopAndClear() {
  SyncServiceImpl::StopAndClear();
  brave_sync_prefs_.Clear();
}

std::string BraveSyncServiceImpl::GetOrCreateSyncCode() {
  bool failed_to_decrypt = false;
  std::string sync_code = brave_sync_prefs_.GetSeed(&failed_to_decrypt);

  if (failed_to_decrypt) {
    // Do not try to re-create seed when OSCrypt fails, for example on macOS
    // when the keyring is locked.
    DCHECK(sync_code.empty());
    return std::string();
  }

  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
  }

  CHECK(!sync_code.empty()) << "Attempt to return empty sync code";
  CHECK(brave_sync::crypto::IsPassphraseValid(sync_code))
      << "Attempt to return non-valid sync code";

  return sync_code;
}

bool BraveSyncServiceImpl::SetSyncCode(const std::string& sync_code) {
  std::string sync_code_trimmed;
  base::TrimString(sync_code, " \n\t", &sync_code_trimmed);
  if (!brave_sync::crypto::IsPassphraseValid(sync_code_trimmed))
    return false;
  if (!brave_sync_prefs_.SetSeed(sync_code_trimmed))
    return false;
  return true;
}

void BraveSyncServiceImpl::OnSelfDeviceInfoDeleted(base::OnceClosure cb) {
  // This function will follow normal reset process and set SyncRequested to
  // false
  StopAndClear();
  std::move(cb).Run();
}

BraveSyncAuthManager* BraveSyncServiceImpl::GetBraveSyncAuthManager() {
  return static_cast<BraveSyncAuthManager*>(auth_manager_.get());
}

void BraveSyncServiceImpl::OnBraveSyncPrefsChanged(const std::string& path) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (path == brave_sync::Prefs::GetSeedPath()) {
    bool failed_to_decrypt = false;
    const std::string seed = brave_sync_prefs_.GetSeed(&failed_to_decrypt);
    DCHECK(!failed_to_decrypt);

    if (!seed.empty()) {
      GetBraveSyncAuthManager()->DeriveSigningKeys(seed);
      // Default enabled types: Bookmarks
      syncer::UserSelectableTypeSet selected_types;
      selected_types.Put(UserSelectableType::kBookmarks);
      GetUserSettings()->SetSelectedTypes(false, selected_types);
    } else {
      VLOG(1) << "Brave sync seed cleared";
      GetBraveSyncAuthManager()->ResetKeys();
      // Send updated status here, because OnDeviceInfoChange is not triggered
      // when device leaves the chain by `Leave Sync Chain` button
      // 0 means disabled or 1 device
      base::UmaHistogramExactLinear("Brave.Sync.Status.2", 0, 3);
    }
  }
}

void BraveSyncServiceImpl::SuspendDeviceObserverForOwnReset() {
  sync_service_impl_delegate_->SuspendDeviceObserverForOwnReset();
}

void BraveSyncServiceImpl::ResumeDeviceObserver() {
  sync_service_impl_delegate_->ResumeDeviceObserver();
}

void BraveSyncServiceImpl::OnEngineInitialized(
    bool success,
    bool is_first_time_sync_configure) {
  SyncServiceImpl::OnEngineInitialized(success, is_first_time_sync_configure);
  if (!IsEngineInitialized()) {
    return;
  }

  syncer::SyncUserSettings* sync_user_settings = GetUserSettings();
  if (!sync_user_settings->IsFirstSetupComplete()) {
    // If first setup has not been complete, we don't need to force
    return;
  }

  bool failed_to_decrypt = false;
  std::string passphrase = brave_sync_prefs_.GetSeed(&failed_to_decrypt);
  DCHECK(!failed_to_decrypt);
  if (passphrase.empty()) {
    return;
  }

  if (sync_user_settings->IsPassphraseRequired()) {
    bool set_passphrase_result =
        sync_user_settings->SetDecryptionPassphrase(passphrase);
    VLOG(1) << "Forced set decryption passphrase result is "
            << set_passphrase_result;
  }
}

SyncServiceCrypto* BraveSyncServiceImpl::GetCryptoForTests() {
  return &crypto_;
}

}  // namespace syncer
