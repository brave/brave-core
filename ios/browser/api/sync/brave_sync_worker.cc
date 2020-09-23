/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/brave_sync_worker.h"

#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "components/sync/driver/profile_sync_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/profile_sync_service_factory.h"
#include "brave/components/sync/driver/brave_sync_profile_sync_service.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/web/public/thread/web_thread.h"

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

  if (!setup_service || !sync_service)
    return false;

  if (!sync_service_observer_.IsObserving(sync_service))
    sync_service_observer_.Add(sync_service);

  setup_service->SetSyncEnabled(enabled);

  if (enabled && !sync_service->GetUserSettings()->IsFirstSetupComplete())
    setup_service->PrepareForFirstSyncSetup();

  return true;
}

const syncer::DeviceInfo* BraveSyncWorker::GetLocalDeviceInfo() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service)
    return nullptr;

  return device_info_service->GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();
}

std::vector<std::unique_ptr<syncer::DeviceInfo>>
BraveSyncWorker::GetDeviceList() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service)
    return std::vector<std::unique_ptr<syncer::DeviceInfo>>();

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

bool BraveSyncWorker::SetSyncCode(const std::string& sync_code) {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  if (sync_code.empty())
    return false;

  auto* sync_service = GetSyncService();
  if (!sync_service || !sync_service->SetSyncCode(sync_code)) {
    return false;
  }
  return true;
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

  if (!tracker)
    return false;
    
  tracker->DeleteDeviceInfo(local_device_info->guid(),
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
  if (!service || !service->IsEngineInitialized())
    return;

  if (IsSyncFeatureActive())
    LOG(ERROR) << "OMFG it worked!!!";

  auto* setup_service =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);

  if (setup_service && !service->GetUserSettings()->IsFirstSetupComplete()) {
    brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
    std::string sync_code = brave_sync_prefs.GetSeed();
    service->GetUserSettings()->EnableEncryptEverything();
    service->GetUserSettings()->SetEncryptionPassphrase(sync_code);
    setup_service->SetFirstSetupComplete(
        syncer::SyncFirstSetupCompleteSource::ADVANCED_FLOW_CONFIRM);
  }
}

void BraveSyncWorker::OnSyncShutdown(syncer::SyncService* service) {
  if (sync_service_observer_.IsObserving(service))
    sync_service_observer_.Remove(service);
}

void BraveSyncWorker::OnLocalDeviceInfoDeleted() {
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (sync_service)
    sync_service->StopAndClear();

  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  brave_sync_prefs.Clear();
}

bool BraveSyncWorker::IsSyncEnabled() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* setup_service =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);

  if (!setup_service)
    return false;

  return setup_service->IsSyncEnabled();
}

bool BraveSyncWorker::IsSyncFeatureActive() {
  DCHECK_CURRENTLY_ON(web::WebThread::UI);
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!sync_service)
    return false;

  return sync_service->IsSyncFeatureActive();
}
