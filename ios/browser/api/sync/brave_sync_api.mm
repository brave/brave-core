/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/brave_sync_api.h"

#import <CoreImage/CoreImage.h>
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
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class BraveSyncWorker : public syncer::SyncServiceObserver {
 public:
  BraveSyncWorker(ChromeBrowserState* browser_state_);
  ~BraveSyncWorker() override;

  bool SetSyncEnabled(bool enabled);
  std::string GetOrCreateSyncCode();
  bool SetSyncCode(const std::string& sync_code);
  const syncer::DeviceInfo* GetLocalDeviceInfo();
  std::vector<std::unique_ptr<syncer::DeviceInfo>> GetDeviceList();
  bool IsSyncEnabled();
  bool IsSyncFeatureActive();
  bool ResetSync();

 private:
  // syncer::SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* service) override;
  void OnSyncShutdown(syncer::SyncService* service) override;

  void OnLocalDeviceInfoDeleted();

  ChromeBrowserState* browser_state_;  // NOT OWNED
  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      sync_service_observer_{this};
  base::WeakPtrFactory<BraveSyncWorker> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(BraveSyncWorker);
};

BraveSyncWorker::BraveSyncWorker(ChromeBrowserState* browser_state)
    : browser_state_(browser_state) {}

BraveSyncWorker::~BraveSyncWorker() {
  // Observer will be removed by ScopedObserver
}

bool BraveSyncWorker::SetSyncEnabled(bool enabled) {
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
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service)
    return nullptr;

  return device_info_service->GetLocalDeviceInfoProvider()->GetLocalDeviceInfo();
}

std::vector<std::unique_ptr<syncer::DeviceInfo>>
BraveSyncWorker::GetDeviceList() {
  auto* device_info_service =
      DeviceInfoSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!device_info_service)
    return std::vector<std::unique_ptr<syncer::DeviceInfo>>();

  syncer::DeviceInfoTracker* tracker =
      device_info_service->GetDeviceInfoTracker();
  return tracker->GetAllDeviceInfo();
}

std::string BraveSyncWorker::GetOrCreateSyncCode() {
  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  std::string sync_code = brave_sync_prefs.GetSeed();
  if (sync_code.empty()) {
    std::vector<uint8_t> seed = brave_sync::crypto::GetSeed();
    sync_code = brave_sync::crypto::PassphraseFromBytes32(seed);
    brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
    brave_sync_prefs.SetSeed(sync_code);
  }
  return sync_code;
}

bool BraveSyncWorker::SetSyncCode(const std::string& sync_code) {
  if (sync_code.empty())
    return false;

  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);
  if (sync_service->GetUserSettings()->IsFirstSetupComplete())
    return false;

  std::vector<uint8_t> seed;
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed))
    return false;

  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  brave_sync_prefs.SetSeed(sync_code);
  return true;
}

bool BraveSyncWorker::ResetSync() {
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

void BraveSyncWorker::OnLocalDeviceInfoDeleted() {
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (sync_service)
    sync_service->StopAndClear();

  brave_sync::Prefs brave_sync_prefs(browser_state_->GetPrefs());
  brave_sync_prefs.Clear();
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

bool BraveSyncWorker::IsSyncEnabled() {
  auto* setup_service =
      SyncSetupServiceFactory::GetForBrowserState(browser_state_);

  if (!setup_service)
    return false;

  return setup_service->IsSyncEnabled();
}

bool BraveSyncWorker::IsSyncFeatureActive() {
  auto* sync_service =
      ProfileSyncServiceFactory::GetForBrowserState(browser_state_);

  if (!sync_service)
    return false;

  return sync_service->IsSyncFeatureActive();
}

}  // namespace

@interface BraveSyncAPI()
{
  std::unique_ptr<BraveSyncWorker> _worker;
}
@end

@implementation BraveSyncAPI

- (instancetype)init {
  if ((self = [super init])) {
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    ChromeBrowserState* chromeBrowserState =
        browserStateManager->GetLastUsedBrowserState();
    _worker.reset(new BraveSyncWorker(chromeBrowserState));
  }
  return self;
}

- (void)dealloc {
  _worker.reset();
}

- (bool)setSyncEnabled:(bool)enabled {
  return _worker->SetSyncEnabled(enabled);
}

- (NSString *)getOrCreateSyncCode {
  std::string syncCode = _worker->GetOrCreateSyncCode();
  if (syncCode.empty())
    return nil;

  return base::SysUTF8ToNSString(syncCode);
}

- (bool)setSyncCode:(NSString *)syncCode {
  return _worker->SetSyncCode(base::SysNSStringToUTF8(syncCode));
}

- (UIImage *)getQRCodeImage:(CGSize)size {
  std::vector<uint8_t> seed;
  std::string sync_code = _worker->GetOrCreateSyncCode();
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed)) {
    return nil;
  }

  // QR code version 3 can only carry 84 bytes so we hex encode 32 bytes
  // seed then we will have 64 bytes input data
  const std::string sync_code_hex = base::HexEncode(seed.data(), seed.size());

  NSData *sync_code_data = [base::SysUTF8ToNSString(sync_code_hex.c_str())
      dataUsingEncoding: NSUTF8StringEncoding]; //NSISOLatin1StringEncoding

  if (!sync_code_data) {
    return nil;
  }

  CIFilter *filter = [CIFilter filterWithName:@"CIQRCodeGenerator"];
  [filter setValue:sync_code_data forKey:@"inputMessage"];
  [filter setValue:@"H" forKey:@"inputCorrectionLevel"];

  CIImage *ciImage = [filter outputImage];
  if (ciImage) {
    CGFloat scaleX = size.width / ciImage.extent.size.width;
    CGFloat scaleY = size.height / ciImage.extent.size.height;
    CGAffineTransform transform = CGAffineTransformMakeScale(scaleX, scaleY);
    ciImage = [ciImage imageByApplyingTransform:transform];
  } else {
    return nil;
  }

  return [UIImage imageWithCIImage:ciImage
                  scale:[[UIScreen mainScreen] scale]
                  orientation:UIImageOrientationUp];
}

- (NSString *)getDeviceListJSON {
    auto deviceList = _worker->GetDeviceList();
    auto* localDeviecInfo = _worker->GetLocalDeviceInfo();

    base::Value deviceListValue(base::Value::Type::LIST);

    fprintf(stderr, "DEVICE INFO: %p\n", localDeviecInfo);
    for (const auto& device : deviceList) {
        auto deviceValue = base::Value::FromUniquePtrValue(device->ToValue());
        bool is_current_device = localDeviecInfo
            ? localDeviecInfo->guid() == device->guid()
            : false;
        deviceValue.SetBoolKey("isCurrentDevice", is_current_device);
        deviceListValue.Append(std::move(deviceValue));
    }

    std::string json_string;
    if (!base::JSONWriter::Write(deviceListValue, &json_string)) {
      return nil;
    }

    return base::SysUTF8ToNSString(json_string);
}

- (bool)resetSync {
  return _worker->ResetSync();
}

- (bool)isSyncEnabled {
  return _worker->IsSyncEnabled();
}

- (bool)isSyncFeatureActive {
  return _worker->IsSyncFeatureActive();
}
@end
