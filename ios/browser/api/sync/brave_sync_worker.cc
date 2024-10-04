/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/sync/brave_sync_worker.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/qr_code_data.h"
#include "brave/components/brave_sync/qr_code_validator.h"
#include "brave/components/brave_sync/sync_service_impl_helper.h"
#include "brave/components/brave_sync/time_limited_words.h"
#include "brave/components/sync/service/brave_sync_service_impl.h"
#include "brave/components/sync_device_info/brave_device_info.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_service_impl.h"
#include "components/sync/service/sync_service_observer.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/sync/model/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/web/public/thread/web_thread.h"

namespace {
static const size_t SEED_BYTES_COUNT = 32u;
}  // namespace

BraveSyncDeviceTracker::BraveSyncDeviceTracker(
    syncer::DeviceInfoTracker* device_info_tracker,
    const base::RepeatingCallback<void()>& on_device_info_changed_callback)
    : on_device_info_changed_callback_(on_device_info_changed_callback) {
  DCHECK(device_info_tracker);
  device_info_tracker_observer_.Observe(device_info_tracker);
}

BraveSyncDeviceTracker::~BraveSyncDeviceTracker() {
  // Observer will be removed by ScopedObservation
}

void BraveSyncDeviceTracker::OnDeviceInfoChange() {
  if (on_device_info_changed_callback_) {
    on_device_info_changed_callback_.Run();
  }
}

BraveSyncServiceTracker::BraveSyncServiceTracker(
    syncer::SyncServiceImpl* sync_service_impl,
    const base::RepeatingCallback<void()>& on_state_changed_callback,
    const base::RepeatingCallback<void()>& on_sync_shutdown_callback)
    : on_state_changed_callback_(on_state_changed_callback),
      on_sync_shutdown_callback_(on_sync_shutdown_callback) {
  DCHECK(sync_service_impl);
  sync_service_observer_.Observe(sync_service_impl);
}

BraveSyncServiceTracker::~BraveSyncServiceTracker() {
  // Observer will be removed by ScopedObservation
}

void BraveSyncServiceTracker::OnStateChanged(syncer::SyncService* sync) {
  if (on_state_changed_callback_) {
    on_state_changed_callback_.Run();
  }
}

void BraveSyncServiceTracker::OnSyncShutdown(syncer::SyncService* sync) {
  if (on_sync_shutdown_callback_) {
    on_sync_shutdown_callback_.Run();
  }
}

BraveSyncWorker::BraveSyncWorker(ChromeBrowserState* browser_state)
    : browser_state_(browser_state) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

BraveSyncWorker::~BraveSyncWorker() {
  // Observer will be removed by ScopedObservation
}

bool BraveSyncWorker::RequestSync() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::SyncService* sync_service = GetSyncService();

  if (!sync_service) {
    return false;
  }

  if (!sync_service_observer_.IsObservingSource(sync_service)) {
    sync_service_observer_.AddObservation(sync_service);
  }

  sync_service->SetSyncFeatureRequested();

  return true;
}

const syncer::DeviceInfo* BraveSyncWorker::GetLocalDeviceInfo() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service) {
    return nullptr;
  }

  return device_info_service->GetLocalDeviceInfoProvider()
      ->GetLocalDeviceInfo();
}

std::vector<std::unique_ptr<syncer::BraveDeviceInfo>>
BraveSyncWorker::GetDeviceList() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service) {
    return std::vector<std::unique_ptr<syncer::BraveDeviceInfo>>();
  }

  syncer::DeviceInfoTracker* tracker =
      device_info_service->GetDeviceInfoTracker();
  return tracker->GetAllBraveDeviceInfo();
}

std::string BraveSyncWorker::GetOrCreateSyncCode() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service) {
    sync_code = sync_service->GetOrCreateSyncCode();
  }

  CHECK(brave_sync::crypto::IsPassphraseValid(sync_code));
  return sync_code;
}

bool BraveSyncWorker::IsValidSyncCode(const std::string& sync_code) {
  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed)) {
    return false;
  }
  return seed.size() == SEED_BYTES_COUNT;
}

bool BraveSyncWorker::SetSyncCode(const std::string& sync_code) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if (sync_code.empty()) {
    return false;
  }

  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(sync_code)) {
    const std::string error_msg = sync_service
                                      ? "invalid sync code:" + sync_code
                                      : "sync service is not available";
    LOG(ERROR) << error_msg;
    return false;
  }

  passphrase_ = sync_code;
  return true;
}

std::string BraveSyncWorker::GetSyncCodeFromHexSeed(
    const std::string& hex_code_seed) {
  DCHECK(!hex_code_seed.empty());

  std::vector<uint8_t> bytes;
  std::string sync_code_words;
  if (base::HexStringToBytes(hex_code_seed, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    if (bytes.size() == SEED_BYTES_COUNT) {
      sync_code_words = brave_sync::crypto::PassphraseFromBytes32(bytes);
      if (sync_code_words.empty()) {
        VLOG(1) << __func__ << " PassphraseFromBytes32 failed for "
                << hex_code_seed;
      }
    } else {
      LOG(ERROR) << "wrong seed bytes " << bytes.size();
    }

    DCHECK_NE(sync_code_words, "");
  } else {
    VLOG(1) << __func__ << " HexStringToBytes failed for hex_code_seed";
  }
  return sync_code_words;
}

std::string BraveSyncWorker::GetHexSeedFromSyncCode(
    const std::string& code_words) {
  DCHECK(!code_words.empty());

  std::string sync_code_hex;
  std::vector<uint8_t> bytes;
  if (brave_sync::crypto::PassphraseToBytes32(code_words, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    if (bytes.size() == SEED_BYTES_COUNT) {
      sync_code_hex = base::HexEncode(&bytes.at(0), bytes.size());
    } else {
      LOG(ERROR) << "wrong seed bytes " << bytes.size();
    }
  } else {
    VLOG(1) << __func__ << " PassphraseToBytes32 failed for " << code_words;
  }
  return sync_code_hex;
}

std::string BraveSyncWorker::GetQrCodeJsonFromHexSeed(
    const std::string& hex_seed) {
  DCHECK(!hex_seed.empty());
  return brave_sync::QrCodeData::CreateWithActualDate(hex_seed)->ToJson();
}

brave_sync::QrCodeDataValidationResult
BraveSyncWorker::GetQrCodeValidationResult(const std::string& json) {
  DCHECK(!json.empty());
  return brave_sync::QrCodeDataValidator::ValidateQrDataJson(json);
}

brave_sync::TimeLimitedWords::ValidationStatus
BraveSyncWorker::GetWordsValidationResult(
    const std::string& time_limited_words) {
  DCHECK(!time_limited_words.empty());
  auto words_with_status =
      brave_sync::TimeLimitedWords::Parse(time_limited_words);
  if (words_with_status.has_value()) {
    return brave_sync::TimeLimitedWords::ValidationStatus::kValid;
  } else {
    return words_with_status.error();
  }
}

std::string BraveSyncWorker::GetWordsFromTimeLimitedWords(
    const std::string& time_limited_words) {
  DCHECK(!time_limited_words.empty());
  auto words_with_status =
      brave_sync::TimeLimitedWords::Parse(time_limited_words);
  DCHECK(words_with_status.has_value());
  return words_with_status.value();
}

std::string BraveSyncWorker::GetTimeLimitedWordsFromWords(
    const std::string& words) {
  DCHECK(!words.empty());
  auto generate_result = brave_sync::TimeLimitedWords::GenerateForNow(words);
  if (generate_result.has_value()) {
    return generate_result.value();
  } else {
    DCHECK(false);
    return std::string();
  }
}

std::string BraveSyncWorker::GetHexSeedFromQrCodeJson(const std::string& json) {
  DCHECK(!json.empty());
  std::unique_ptr<brave_sync::QrCodeData> qr_data =
      brave_sync::QrCodeData::FromJson(json);
  if (qr_data) {
    DCHECK(!GetSyncCodeFromHexSeed(qr_data->sync_code_hex).empty());
    return qr_data->sync_code_hex;
  }

  DCHECK(!GetSyncCodeFromHexSeed(json).empty());
  return json;
}

bool BraveSyncWorker::IsInitialSyncFeatureSetupComplete() {
  syncer::SyncService* sync_service = GetSyncService();
  return sync_service &&
         sync_service->GetUserSettings()->IsInitialSyncFeatureSetupComplete();
}

bool BraveSyncWorker::SetSetupComplete() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::SyncService* sync_service = GetSyncService();

  if (!sync_service) {
    return false;
  }

  sync_service->SetSyncFeatureRequested();

  if (!sync_service->GetUserSettings()->IsInitialSyncFeatureSetupComplete()) {
    sync_service->GetUserSettings()->SetInitialSyncFeatureSetupComplete(
        syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  }

  return true;
}

void BraveSyncWorker::ResetSync() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();

  if (!sync_service) {
    return;
  }

  sync_service->prefs().AddLeaveChainDetail(__FILE__, __LINE__, __func__);

  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  DCHECK(device_info_service);

  brave_sync::ResetSync(sync_service, device_info_service,
                        base::BindOnce(&BraveSyncWorker::OnResetDone,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void BraveSyncWorker::DeleteDevice(const std::string& device_guid) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();

  if (!sync_service) {
    return;
  }

  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  DCHECK(device_info_service);

  brave_sync::DeleteDevice(sync_service, device_info_service, device_guid);
}

void BraveSyncWorker::SetJoinSyncChainCallback(
    base::OnceCallback<void(bool)> callback) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();

  if (!sync_service) {
    return;
  }

  sync_service->SetJoinChainResultCallback(std::move(callback));
}

void BraveSyncWorker::PermanentlyDeleteAccount(
    base::OnceCallback<void(const syncer::SyncProtocolError&)> callback) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::BraveSyncServiceImpl* sync_service = GetSyncService();

  if (!sync_service) {
    return;
  }

  sync_service->prefs().AddLeaveChainDetail(__FILE__, __LINE__, __func__);

  sync_service->PermanentlyDeleteAccount(std::move(callback));
}

syncer::BraveSyncServiceImpl* BraveSyncWorker::GetSyncService() const {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return static_cast<syncer::BraveSyncServiceImpl*>(
      SyncServiceFactory::GetForBrowserState(browser_state_));
}

void BraveSyncWorker::SetEncryptionPassphrase(syncer::SyncService* service) {
  DCHECK(service);
  DCHECK(service->IsEngineInitialized());
  DCHECK(!this->passphrase_.empty());

  syncer::SyncUserSettings* sync_user_settings = service->GetUserSettings();
  DCHECK(!sync_user_settings->IsPassphraseRequired());

  if (sync_user_settings->IsCustomPassphraseAllowed() &&
      !sync_user_settings->IsUsingExplicitPassphrase() &&
      !sync_user_settings->IsTrustedVaultKeyRequired()) {
    sync_user_settings->SetEncryptionPassphrase(this->passphrase_);

    VLOG(3) << "[BraveSync] " << __func__ << " SYNC_CREATED_NEW_PASSPHRASE";
  }
}

void BraveSyncWorker::SetDecryptionPassphrase(syncer::SyncService* service) {
  DCHECK(service);
  DCHECK(service->IsEngineInitialized());
  DCHECK(!this->passphrase_.empty());

  syncer::SyncUserSettings* sync_user_settings = service->GetUserSettings();
  DCHECK(sync_user_settings->IsPassphraseRequired());

  if (sync_user_settings->SetDecryptionPassphrase(this->passphrase_)) {
    VLOG(3) << "[BraveSync] " << __func__
            << " SYNC_ENTERED_EXISTING_PASSPHRASE";
  }
}

void BraveSyncWorker::OnStateChanged(syncer::SyncService* service) {
  // If the sync engine has shutdown for some reason, just give up
  if (!service || !service->IsEngineInitialized()) {
    VLOG(3) << "[BraveSync] " << __func__ << " sync engine is not initialized";
    return;
  }

  if (this->passphrase_.empty()) {
    VLOG(3) << "[BraveSync] " << __func__ << " empty passphrase";
    return;
  }

  if (service->GetUserSettings()->IsPassphraseRequired()) {
    SetDecryptionPassphrase(service);
  } else {
    SetEncryptionPassphrase(service);
  }
}

void BraveSyncWorker::OnSyncShutdown(syncer::SyncService* service) {
  if (sync_service_observer_.IsObservingSource(service)) {
    sync_service_observer_.RemoveObservation(service);
  }
}

void BraveSyncWorker::OnResetDone() {
  syncer::SyncService* sync_service = GetSyncService();
  if (sync_service && sync_service_observer_.IsObservingSource(sync_service)) {
    sync_service_observer_.RemoveObservation(sync_service);
  }
}

bool BraveSyncWorker::CanSyncFeatureStart() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::SyncService* sync_service = GetSyncService();

  if (!sync_service) {
    return false;
  }

  return sync_service->IsSyncFeatureEnabled();
}

bool BraveSyncWorker::IsSyncFeatureActive() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  syncer::SyncService* sync_service = GetSyncService();

  if (!sync_service) {
    return false;
  }

  return sync_service->IsSyncFeatureActive();
}
