/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/service/brave_sync_service_impl.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_sync/brave_sync_p3a.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/service/brave_sync_auth_manager.h"
#include "brave/components/sync/service/sync_service_impl_delegate.h"
#include "components/prefs/pref_service.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/model/type_entities_count.h"

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
  if (!user_settings_->IsInitialSyncFeatureSetupComplete()) {
    base::UmaHistogramExactLinear("Brave.Sync.Status.2", 0, 3);
  }
}

bool BraveSyncServiceImpl::IsSetupInProgress() const {
  return SyncServiceImpl::IsSetupInProgress() &&
         !user_settings_->IsInitialSyncFeatureSetupComplete();
}

void BraveSyncServiceImpl::StopAndClear() {
  // Clear prefs before StopAndClear() to make NotifyObservers() be invoked
  brave_sync_prefs_.Clear();
  SyncServiceImpl::StopAndClear();
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
  if (!brave_sync::crypto::IsPassphraseValid(sync_code_trimmed)) {
    return false;
  }
  if (!brave_sync_prefs_.SetSeed(sync_code_trimmed)) {
    return false;
  }

  initiated_delete_account_ = false;
  initiated_self_device_info_deleted_ = false;
  initiated_join_chain_ = true;

  return true;
}

void BraveSyncServiceImpl::OnSelfDeviceInfoDeleted(base::OnceClosure cb) {
  initiated_self_device_info_deleted_ = true;
  // This function will follow normal reset process and set SyncRequested to
  // false

  // We need this to avoid |StopAndClear| call below when initiating sync
  // chain after clear data when the sync passphrase wasn't decrypted.
  // Otherwise we have these calls:
  // ---
  // BraveSyncServiceImplDelegate::OnDeviceInfoChange()
  // ...
  // ClientTagBasedModelTypeProcessor::ClearAllMetadataAndResetStateImpl()
  // ...
  // ClientTagBasedModelTypeProcessor::OnSyncStarting()
  // ---
  // Note that `ClearAllTrackedMetadataAndResetState` will only be called during
  // init when sync seed decryption key mismatched.
  if (GetTransportState() != TransportState::CONFIGURING) {
    StopAndClear();
  }

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
  if (!sync_user_settings->IsInitialSyncFeatureSetupComplete()) {
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

namespace {
const int kMaxPermanentlyDeleteSyncAccountAttempts = 5;
const int kDelayBetweenDeleteSyncAccountAttemptsMsec = 500;
}  // namespace

void BraveSyncServiceImpl::OnAccountDeleted(
    const int current_attempt,
    base::OnceCallback<void(const SyncProtocolError&)> callback,
    const SyncProtocolError& sync_protocol_error) {
  if (sync_protocol_error.error_type == SYNC_SUCCESS) {
    std::move(callback).Run(sync_protocol_error);
    // If request succeded - reset and clear all in a forced way
    // The code below cleans all on an initiator device. Other devices in the
    // chain will be cleaned at BraveSyncServiceImpl::ResetEngine
    DCHECK(initiated_delete_account_);
    BraveSyncServiceImpl::StopAndClear();
  } else if (current_attempt < kMaxPermanentlyDeleteSyncAccountAttempts) {
    // Server responded failure, but we need to try more
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&BraveSyncServiceImpl::PermanentlyDeleteAccountImpl,
                       weak_ptr_factory_.GetWeakPtr(), current_attempt + 1,
                       std::move(callback)),
        base::Milliseconds(kDelayBetweenDeleteSyncAccountAttemptsMsec));
  } else {
    // Server responded failure, and we are out of our attempts
    initiated_delete_account_ = false;
    std::move(callback).Run(sync_protocol_error);
  }
}

void BraveSyncServiceImpl::PermanentlyDeleteAccountImpl(
    const int current_attempt,
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  if (!engine_) {
    // We can reach here if two devices almost at the same time will initiate
    // the deletion procedure
    SyncProtocolError sync_protocol_error;
    sync_protocol_error.error_type = SYNC_SUCCESS;
    std::move(callback).Run(sync_protocol_error);
    return;
  }

  DCHECK_GE(current_attempt, 1);
  DCHECK_NE(current_attempt, 10);

  engine_->PermanentlyDeleteAccount(base::BindOnce(
      &BraveSyncServiceImpl::OnAccountDeleted, weak_ptr_factory_.GetWeakPtr(),
      current_attempt, std::move(callback)));
}

void BraveSyncServiceImpl::PermanentlyDeleteAccount(
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  initiated_delete_account_ = true;
  PermanentlyDeleteAccountImpl(1, std::move(callback));
}

void BraveSyncServiceImpl::ResetEngine(ShutdownReason shutdown_reason,
                                       ResetEngineReason reset_reason) {
  SyncServiceImpl::ResetEngine(shutdown_reason, reset_reason);

  if (initiated_self_device_info_deleted_) {
    return;
  }

  if (shutdown_reason == ShutdownReason::DISABLE_SYNC_AND_CLEAR_DATA &&
      reset_reason == ResetEngineReason::kDisabledAccount &&
      sync_disabled_by_admin_ && !initiated_delete_account_ &&
      !initiated_join_chain_) {
    brave_sync_prefs_.SetSyncAccountDeletedNoticePending(true);
    // Forcing stop and clear, because sync account was deleted
    BraveSyncServiceImpl::StopAndClear();
  } else if (shutdown_reason == ShutdownReason::DISABLE_SYNC_AND_CLEAR_DATA &&
             reset_reason == ResetEngineReason::kDisabledAccount &&
             sync_disabled_by_admin_ && initiated_join_chain_) {
    // Forcing stop and clear, because we are trying to join the sync chain, but
    // sync account was deleted
    BraveSyncServiceImpl::StopAndClear();
    // When it will be merged into master, iOS code will be a bit behind,
    // so don't expect join_chain_result_callback_ is set, but get CHECK back
    // once iOS changes will handle this
    LOG_IF(ERROR, !join_chain_result_callback_)
        << "[BraveSync] " << __func__
        << " join_chain_result_callback_ must be set";
    if (join_chain_result_callback_) {
      std::move(join_chain_result_callback_).Run(false);
    }
  }
}

void BraveSyncServiceImpl::SetJoinChainResultCallback(
    base::OnceCallback<void(bool)> callback) {
  join_chain_result_callback_ = std::move(callback);

  sync_service_impl_delegate_->SetLocalDeviceAppearedCallback(
      base::BindOnce(&BraveSyncServiceImpl::LocalDeviceAppeared,
                     weak_ptr_factory_.GetWeakPtr()));
}

void BraveSyncServiceImpl::LocalDeviceAppeared() {
  initiated_join_chain_ = false;
  DCHECK(join_chain_result_callback_);
  std::move(join_chain_result_callback_).Run(true);
  SyncServiceImpl::NotifyObservers();
}

namespace {
const int kCyclesBeforeUpdateP3AObjects = 10;
}  // namespace

void BraveSyncServiceImpl::OnSyncCycleCompleted(
    const SyncCycleSnapshot& snapshot) {
  SyncServiceImpl::OnSyncCycleCompleted(snapshot);
  if (completed_cycles_count_ % kCyclesBeforeUpdateP3AObjects == 0) {
    UpdateP3AObjectsNumber();
  }
  ++completed_cycles_count_;
}

void BraveSyncServiceImpl::UpdateP3AObjectsNumber() {
  GetEntityCountsForDebugging(BindOnce(&BraveSyncServiceImpl::OnGotEntityCounts,
                                       weak_ptr_factory_.GetWeakPtr()));
}

void BraveSyncServiceImpl::OnGotEntityCounts(
    const std::vector<syncer::TypeEntitiesCount>& entity_counts) {
  int total_entities = 0;
  for (const syncer::TypeEntitiesCount& count : entity_counts) {
    total_entities += count.non_tombstone_entities;
  }

  brave_sync::p3a::RecordSyncedObjectsCount(total_entities);
}

void BraveSyncServiceImpl::OnPreferredDataTypesPrefChange(
    bool payments_integration_enabled_changed) {
  SyncServiceImpl::OnPreferredDataTypesPrefChange(
      payments_integration_enabled_changed);

  brave_sync::p3a::RecordEnabledTypes(
      GetUserSettings()->IsSyncEverythingEnabled(),
      GetUserSettings()->GetSelectedTypes());
}

}  // namespace syncer
