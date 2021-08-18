/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/brave_sync_api.h"

#import <CoreImage/CoreImage.h>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/sync_device_info/brave_device_info.h"
#include "brave/ios/browser/api/sync/brave_sync_worker.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_service_impl.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/sync_device_info/device_info.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"
#include "components/sync_device_info/local_device_info_provider.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"
#include "ios/chrome/browser/sync/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BraveSyncDeviceObserver : NSObject {
  std::unique_ptr<BraveSyncDeviceTracker> _device_observer;
}
@end

@implementation BraveSyncDeviceObserver

- (instancetype)initWithDeviceInfoTracker:(syncer::DeviceInfoTracker*)tracker
                                 callback:(void (^)())onDeviceInfoChanged {
  if ((self = [super init])) {
    _device_observer =
        std::make_unique<BraveSyncDeviceTracker>(tracker, onDeviceInfoChanged);
  }
  return self;
}
@end

@interface BraveSyncServiceObserver : NSObject {
  std::unique_ptr<BraveSyncServiceTracker> _service_tracker;
}
@end

@implementation BraveSyncServiceObserver

- (instancetype)initWithSyncServiceImpl:
                    (syncer::SyncServiceImpl*)syncServiceImpl
                               callback:(void (^)())onSyncServiceStateChanged {
  if ((self = [super init])) {
    _service_tracker = std::make_unique<BraveSyncServiceTracker>(
        syncServiceImpl, onSyncServiceStateChanged);
  }
  return self;
}
@end

@interface BraveSyncAPI () {
  std::unique_ptr<BraveSyncWorker> _worker;
  ChromeBrowserState* _chromeBrowserState;
}
@end

@implementation BraveSyncAPI

+ (instancetype)sharedSyncAPI {
  static BraveSyncAPI* instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[BraveSyncAPI alloc] init];
  });
  return instance;
}

- (instancetype)init {
  if ((self = [super init])) {
    ios::ChromeBrowserStateManager* browserStateManager =
        GetApplicationContext()->GetChromeBrowserStateManager();
    _chromeBrowserState = browserStateManager->GetLastUsedBrowserState();
    _worker.reset(new BraveSyncWorker(_chromeBrowserState));
  }
  return self;
}

- (void)dealloc {
  _worker.reset();
  _chromeBrowserState = NULL;
}

- (bool)syncEnabled {
  return _worker->CanSyncFeatureStart();
}

- (void)setSyncEnabled:(bool)enabled {
  _worker->SetSyncEnabled(enabled);
}

- (bool)isSyncFeatureActive {
  return _worker->IsSyncFeatureActive();
}

- (bool)isValidSyncCode:(NSString*)syncCode {
  return _worker->IsValidSyncCode(base::SysNSStringToUTF8(syncCode));
}

- (NSString*)getSyncCode {
  std::string syncCode = _worker->GetOrCreateSyncCode();
  if (syncCode.empty()) {
    return nil;
  }

  return base::SysUTF8ToNSString(syncCode);
}

- (bool)setSyncCode:(NSString*)syncCode {
  return _worker->SetSyncCode(base::SysNSStringToUTF8(syncCode));
}

- (NSString*)syncCodeFromHexSeed:(NSString*)hexSeed {
  return base::SysUTF8ToNSString(
      _worker->GetSyncCodeFromHexSeed(base::SysNSStringToUTF8(hexSeed)));
}

- (UIImage*)getQRCodeImage:(CGSize)size {
  std::vector<uint8_t> seed;
  std::string sync_code = _worker->GetOrCreateSyncCode();
  if (!brave_sync::crypto::PassphraseToBytes32(sync_code, &seed)) {
    return nil;
  }

  // QR code version 3 can only carry 84 bytes so we hex encode 32 bytes
  // seed then we will have 64 bytes input data
  const std::string sync_code_hex = base::HexEncode(seed.data(), seed.size());

  NSData* sync_code_data = [base::SysUTF8ToNSString(sync_code_hex.c_str())
      dataUsingEncoding:NSUTF8StringEncoding];  // NSISOLatin1StringEncoding

  if (!sync_code_data) {
    return nil;
  }

  CIFilter* filter = [CIFilter filterWithName:@"CIQRCodeGenerator"];
  [filter setValue:sync_code_data forKey:@"inputMessage"];
  [filter setValue:@"H" forKey:@"inputCorrectionLevel"];

  CIImage* ciImage = [filter outputImage];
  if (ciImage) {
    CGFloat scaleX = size.width / ciImage.extent.size.width;
    CGFloat scaleY = size.height / ciImage.extent.size.height;
    CGAffineTransform transform = CGAffineTransformMakeScale(scaleX, scaleY);
    ciImage = [ciImage imageByApplyingTransform:transform];

    return [UIImage imageWithCIImage:ciImage
                               scale:[[UIScreen mainScreen] scale]
                         orientation:UIImageOrientationUp];
  }
  return nil;
}

- (NSString*)getDeviceListJSON {
  auto device_list = _worker->GetDeviceList();
  auto* local_device_info = _worker->GetLocalDeviceInfo();

  base::Value device_list_value(base::Value::Type::LIST);

  for (const auto& device : device_list) {
    auto device_value = base::Value::FromUniquePtrValue(device->ToValue());
    bool is_current_device =
        local_device_info ? local_device_info->guid() == device->guid() : false;
    device_value.SetBoolKey("isCurrentDevice", is_current_device);
    device_value.SetStringKey("guid", device->guid());
    device_value.SetBoolKey("supportsSelfDelete",
                            device->is_self_delete_supported());
    device_list_value.Append(std::move(device_value));
  }

  std::string json_string;
  if (!base::JSONWriter::Write(device_list_value, &json_string)) {
    return nil;
  }

  return base::SysUTF8ToNSString(json_string);
}

- (void)resetSync {
  _worker->ResetSync();
}

- (void)deleteDevice:(NSString*)guid {
  _worker->DeleteDevice(base::SysNSStringToUTF8(guid));
}

- (id)createSyncDeviceObserver:(void (^)())onDeviceInfoChanged {
  auto* tracker =
      DeviceInfoSyncServiceFactory::GetForBrowserState(_chromeBrowserState)
          ->GetDeviceInfoTracker();
  return [[BraveSyncDeviceObserver alloc]
      initWithDeviceInfoTracker:tracker
                       callback:onDeviceInfoChanged];
}

- (id)createSyncServiceObserver:(void (^)())onSyncServiceStateChanged {
  auto* service = SyncServiceFactory::GetAsSyncServiceImplForBrowserState(
      _chromeBrowserState);
  return [[BraveSyncServiceObserver alloc]
      initWithSyncServiceImpl:service
                     callback:onSyncServiceStateChanged];
}
@end
