/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/sync/brave_sync_worker.h"

#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "components/sync/driver/profile_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/web/public/thread/web_thread.h"

namespace {
static const size_t SEED_BYTES_COUNT = 32u;
}  // namespace

namespace {

// A structure which contains all the configuration information for sync.
struct SyncConfigInfo {
  SyncConfigInfo();
  ~SyncConfigInfo();

  bool encrypt_all;
  bool set_new_passphrase;
};

SyncConfigInfo::SyncConfigInfo()
    : encrypt_all(false), set_new_passphrase(false) {}

SyncConfigInfo::~SyncConfigInfo() {}

// Return false if we are not interested configure encryption
bool FillSyncConfigInfo(syncer::SyncService* service,
                        SyncConfigInfo* configuration) {
  bool first_setup_in_progress =
      service && !service->GetUserSettings()->IsFirstSetupComplete();

  configuration->encrypt_all =
      service->GetUserSettings()->IsEncryptEverythingEnabled();

  bool sync_prefs_passphrase_required =
      service->GetUserSettings()->IsPassphraseRequired();

  if (!first_setup_in_progress) {
    if (!configuration->encrypt_all) {
      configuration->encrypt_all = true;
      configuration->set_new_passphrase = true;
    } else if (sync_prefs_passphrase_required) {
      configuration->set_new_passphrase = false;
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace

BraveSyncDeviceTracker::BraveSyncDeviceTracker(
    syncer::DeviceInfoTracker* device_info_tracker,
    std::function<void()> on_device_info_changed_callback)
    : on_device_info_changed_callback_(on_device_info_changed_callback) {
  DCHECK(device_info_tracker);
  device_info_tracker_observer_.Add(device_info_tracker);
}

BraveSyncDeviceTracker::~BraveSyncDeviceTracker() {
  // Observer will be removed by ScopedObserver
}

void BraveSyncDeviceTracker::OnDeviceInfoChange() {
  if (on_device_info_changed_callback_) {
    on_device_info_changed_callback_();
  }
}

BraveSyncServiceTracker::BraveSyncServiceTracker(
    syncer::ProfileSyncService* profile_sync_service,
    std::function<void()> on_state_changed_callback)
    : on_state_changed_callback_(on_state_changed_callback) {
  DCHECK(profile_sync_service);
  sync_service_observer_.Add(profile_sync_service);
}

BraveSyncServiceTracker::~BraveSyncServiceTracker() {
  // Observer will be removed by ScopedObserver
}

void BraveSyncServiceTracker::OnStateChanged(syncer::SyncService* sync) {
  if (on_state_changed_callback_) {
    on_state_changed_callback_();
  }
}

BraveSyncWorker::BraveSyncWorker(ChromeBrowserState* browser_state)
    : browser_state_(browser_state) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
}

BraveSyncWorker::~BraveSyncWorker() {
  // Observer will be removed by ScopedObserver
}

bool BraveSyncWorker::SetSyncEnabled(bool enabled) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* setup_service =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!setup_service || !sync_service) {
    return false;
  }

  if (!sync_service_observer_.IsObserving(sync_service)) {
    sync_service_observer_.Add(sync_service);
  }

  setup_service->SetSyncEnabled(enabled);

  if (enabled && !sync_service->GetUserSettings()->IsFirstSetupComplete()) {
    // setup_service->PrepareForFirstSyncSetup();
    // setup_service->CommitSyncChanges();
    setup_service->SetFirstSetupComplete(
        syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  }

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

std::vector<std::unique_ptr<syncer::DeviceInfo>>
BraveSyncWorker::GetDeviceList() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service) {
    return std::vector<std::unique_ptr<syncer::DeviceInfo>>();
  }

  syncer::DeviceInfoTracker* tracker =
      device_info_service->GetDeviceInfoTracker();
  return tracker->GetAllDeviceInfo();
}

std::string BraveSyncWorker::GetOrCreateSyncCode() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* sync_service = GetSyncService();
  std::string sync_code;
  if (sync_service) {
    sync_code = sync_service->GetOrCreateSyncCode();
  }
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

  auto* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(sync_code)) {
    return false;
  }
  return true;
}

std::string BraveSyncWorker::GetSyncCodeFromHexSeed(
    const std::string& hex_code_seed) {
  DCHECK(!hex_code_seed.empty());

  std::vector<uint8_t> bytes;
  std::string sync_code_words;
  if (base::HexStringToBytes(hex_code_seed, &bytes)) {
    DCHECK_EQ(bytes.size(), SEED_BYTES_COUNT);
    if (bytes.size(), SEED_BYTES_COUNT) {
      sync_code_words = brave_sync::crypto::PassphraseFromBytes32(bytes);
      if (sync_code_words.empty()) {
        VLOG(1) << __func__
                << " PassphraseFromBytes32 failed for hex_code_seed";
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

bool BraveSyncWorker::IsFirstSetupComplete() {
  syncer::SyncService* sync_service = GetSyncService();
  return sync_service &&
         sync_service->GetUserSettings()->IsFirstSetupComplete();
}

bool BraveSyncWorker::ResetSync() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  // Do not send self deleted commit if engine is not up and running
  if (!sync_service || sync_service->GetTransportState() !=
                           syncer::SyncService::TransportState::ACTIVE) {
    OnLocalDeviceInfoDeleted();
    return true;
  }

  auto* local_device_info = GetLocalDeviceInfo();
  if (!local_device_info) {
    // May happens when we reset the chain immediately after connection
    VLOG(1) << __func__ << " no local device info, cannot reset sync now";
    return false;
  }

  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);
  auto* tracker = device_info_service->GetDeviceInfoTracker();

  if (!tracker) {
    return false;
  }

  tracker->DeleteDeviceInfo(
      local_device_info->guid(),
      base::BindOnce(&BraveSyncWorker::OnLocalDeviceInfoDeleted,
                     weak_ptr_factory_.GetWeakPtr()));

  return true;
}

syncer::BraveProfileSyncService* BraveSyncWorker::GetSyncService() const {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  return static_cast<syncer::BraveProfileSyncService*>(
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_));
}

void BraveSyncWorker::OnStateChanged(syncer::SyncService* service) {
  // If the sync engine has shutdown for some reason, just give up
  if (!service || !service->IsEngineInitialized()) {
    VLOG(3) << "[BraveSync] " << __func__ << " sync engine is not initialized";
    return;
  }

  SyncConfigInfo configuration = {};
  if (!FillSyncConfigInfo(service, &configuration)) {
    VLOG(3) << "[BraveSync] " << __func__
            << " operations with passphrase are not required";
    return;
  }

  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  std::string sync_code = brave_sync_prefs.GetSeed();
  DCHECK_NE(sync_code.size(), 0u);

  if (!service->GetUserSettings()->IsEncryptEverythingAllowed()) {
    configuration.set_new_passphrase = false;
  }

  bool passphrase_failed = false;
  if (!sync_code.empty()) {
    if (service->GetUserSettings()->IsPassphraseRequired()) {
      passphrase_failed =
          !service->GetUserSettings()->SetDecryptionPassphrase(sync_code);
    } else if (service->GetUserSettings()->IsTrustedVaultKeyRequired()) {
      passphrase_failed = true;
    } else {
      if (configuration.set_new_passphrase &&
          !service->GetUserSettings()->IsUsingSecondaryPassphrase()) {
        service->GetUserSettings()->SetEncryptionPassphrase(sync_code);
      }
    }
  }

  if (passphrase_failed ||
      service->GetUserSettings()->IsPassphraseRequiredForPreferredDataTypes()) {
    VLOG(1) << __func__ << " setup passphrase failed";
  }
}

void BraveSyncWorker::OnSyncShutdown(syncer::SyncService* service) {
  if (sync_service_observer_.IsObserving(service)) {
    sync_service_observer_.Remove(service);
  }
}

void BraveSyncWorker::OnLocalDeviceInfoDeleted() {
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (sync_service) {
    sync_service->StopAndClear();
  }

  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  brave_sync_prefs.Clear();
}

bool BraveSyncWorker::IsSyncEnabled() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* setup_service =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);

  if (!setup_service) {
    return false;
  }

  return setup_service->IsSyncEnabled();
}

bool BraveSyncWorker::IsSyncFeatureActive() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!sync_service) {
    return false;
  }

  return sync_service->IsSyncFeatureActive();
}
