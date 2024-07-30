/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/service/brave_sync_service_impl.h"

#include <utility>
#include <vector>

#include "base/auto_reset.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_sync/brave_sync_p3a.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/service/brave_sync_auth_manager.h"
#include "brave/components/sync/service/sync_service_impl_delegate.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/model/type_entities_count.h"

namespace syncer {

void BraveSyncServiceImpl::SyncedObjectsCountContext::Reset(
    size_t types_requested_init) {
  types_requested = types_requested_init;
  types_responed = 0;
  total_objects_count = 0;
}

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
  base::AutoReset<bool> is_initializing_resetter(&is_initializing_, true);

  SyncServiceImpl::Initialize();

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
  // StopAndClear is invoked during |SyncServiceImpl::Initialize| even if sync
  // is not enabled. This adds lots of useless lines into
  // `brave_sync_v2.diag.leave_chain_details`
  if (!is_initializing_) {
    brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
  }
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
  brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
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

      // Related Chromium change: 33441a0f3f9a591693157f2fd16852ce072e6f9d
      // We need to acquire setup handle before change selected types.
      // See changes at |SyncServiceImpl::GetSyncAccountStateForPrefs| and
      // |SyncUserSettingsImpl::SetSelectedTypes|
      auto sync_blocker = GetSetupInProgressHandle();

      syncer::UserSelectableTypeSet selected_types;
      selected_types.Put(UserSelectableType::kBookmarks);
      GetUserSettings()->SetSelectedTypes(false, selected_types);

      brave_sync_prefs_.ClearLeaveChainDetails();
    } else {
      VLOG(1) << "Brave sync seed cleared";
      brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
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
  brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
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
  brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
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
  brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
  initiated_delete_account_ = true;
  PermanentlyDeleteAccountImpl(1, std::move(callback));
}

std::unique_ptr<SyncEngine> BraveSyncServiceImpl::ResetEngine(
    ResetEngineReason reset_reason) {
  auto result = SyncServiceImpl::ResetEngine(reset_reason);
  auto shutdown_reason =
      SyncServiceImpl::ShutdownReasonForResetEngineReason(reset_reason);

  if (initiated_self_device_info_deleted_) {
    return result;
  }

  if (shutdown_reason == ShutdownReason::DISABLE_SYNC_AND_CLEAR_DATA &&
      reset_reason == ResetEngineReason::kDisabledAccount &&
      sync_disabled_by_admin_ && !initiated_delete_account_ &&
      !initiated_join_chain_) {
    brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
    brave_sync_prefs_.SetSyncAccountDeletedNoticePending(true);
    // Forcing stop and clear, because sync account was deleted
    BraveSyncServiceImpl::StopAndClear();
  } else if (shutdown_reason == ShutdownReason::DISABLE_SYNC_AND_CLEAR_DATA &&
             reset_reason == ResetEngineReason::kDisabledAccount &&
             sync_disabled_by_admin_ && initiated_join_chain_) {
    brave_sync_prefs_.AddLeaveChainDetail(__FILE__, __LINE__, __func__);
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
  return result;
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
// Typical cycle takes 30 sec, let's send P3A updates each ~30 minutes
const int kCyclesBeforeUpdateP3AObjects = 60;
// And Let's do the first update in ~5 minutes after sync start
const int kCyclesBeforeFirstUpdatesP3A = 10;
}  // namespace

void BraveSyncServiceImpl::OnSyncCycleCompleted(
    const SyncCycleSnapshot& snapshot) {
  SyncServiceImpl::OnSyncCycleCompleted(snapshot);
  if (completed_cycles_count_ == kCyclesBeforeFirstUpdatesP3A ||
      completed_cycles_count_ % kCyclesBeforeUpdateP3AObjects == 0) {
    UpdateP3AObjectsNumber();
  }
  ++completed_cycles_count_;
}

void BraveSyncServiceImpl::UpdateP3AObjectsNumber() {
  synced_objects_context_.Reset(GetUserSettings()->GetSelectedTypes().size());

  for (UserSelectableType user_selected_type :
       GetUserSettings()->GetSelectedTypes()) {
    ModelType model_type =
        UserSelectableTypeToCanonicalModelType(user_selected_type);

    auto dtc_it = data_type_manager_->GetControllerMap().find(model_type);
    CHECK(dtc_it != data_type_manager_->GetControllerMap().end())
        << "Missing controller for type " << ModelTypeToDebugString(model_type);

    ModelTypeController* controller = dtc_it->second.get();
    controller->GetTypeEntitiesCount(
        base::BindOnce(&BraveSyncServiceImpl::OnGetTypeEntitiesCount,
                       weak_ptr_factory_.GetWeakPtr()));
  }
}

void BraveSyncServiceImpl::OnGetTypeEntitiesCount(
    const TypeEntitiesCount& count) {
  ++synced_objects_context_.types_responed;
  synced_objects_context_.total_objects_count += count.non_tombstone_entities;
  if (synced_objects_context_.types_responed ==
      synced_objects_context_.types_requested) {
    if (GetUserSettings()->GetSelectedTypes().Has(
            syncer::UserSelectableType::kHistory)) {
      // History stores info about synced objects in a different way than the
      // others types. Issue a separate request to achieve this info
      sync_service_impl_delegate_->GetKnownToSyncHistoryCount(base::BindOnce(
          [](int total_entities, std::pair<bool, int> known_to_sync_count) {
            brave_sync::p3a::RecordSyncedObjectsCount(
                total_entities + known_to_sync_count.second);
          },
          synced_objects_context_.total_objects_count));
    } else {
      brave_sync::p3a::RecordSyncedObjectsCount(
          synced_objects_context_.total_objects_count);
    }
  }
}

void BraveSyncServiceImpl::OnSelectedTypesPrefChange() {
  SyncServiceImpl::OnSelectedTypesPrefChange();

  brave_sync::p3a::RecordEnabledTypes(
      GetUserSettings()->IsSyncEverythingEnabled(),
      GetUserSettings()->GetSelectedTypes());
}

}  // namespace syncer
