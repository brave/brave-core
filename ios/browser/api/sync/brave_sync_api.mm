/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/brave_sync_api.h"
#import "brave/ios/browser/api/sync/brave_sync_worker.h"

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



@interface BraveSyncDeviceObserver()
{
  std::unique_ptr<BraveSyncDeviceTracker> _device_observer;
}
@end

@implementation BraveSyncDeviceObserver

- (instancetype)initWithCallback:(void(^)())onDeviceInfoChanged {
  if ((self = [super init])) {
    _device_observer = std::make_unique<BraveSyncDeviceTracker>(onDeviceInfoChanged);
  }
  return self;
}
@end

@interface BraveSyncServiceObserver()
{
  std::unique_ptr<BraveSyncServiceTracker> _service_observer;
}
@end

@implementation BraveSyncServiceObserver

- (instancetype)initWithCallback:(void(^)())onSyncServiceStateChanged {
  if ((self = [super init])) {
    _service_observer = std::make_unique<BraveSyncServiceTracker>([onSyncServiceStateChanged](syncer::SyncService *sync) {
      onSyncServiceStateChanged();
    }, [](syncer::SyncService *sync) {
        fprintf(stderr, "Sync Shut Down\n");
    });
  }
  return self;
}
@end

@interface BraveSyncAPI()
{
  std::unique_ptr<BraveSyncWorker> _worker;
}
@end

@implementation BraveSyncAPI

+ (instancetype)sharedSyncAPI {
	static BraveSyncAPI *instance = nil;
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

- (bool)isValidSyncCode:(NSString *)syncCode {
  return _worker->IsValidSyncCode(base::SysNSStringToUTF8(syncCode));
}

- (NSString *)getSyncCode {
  std::string syncCode = _worker->GetOrCreateSyncCode();
  if (syncCode.empty()) {
    return nil;
  }

  return base::SysUTF8ToNSString(syncCode);
}

- (bool)setSyncCode:(NSString *)syncCode {
  return _worker->SetSyncCode(base::SysNSStringToUTF8(syncCode));
}

- (NSString *)syncCodeFromHexSeed:(NSString *)hexSeed {
  return base::SysUTF8ToNSString(_worker->GetSyncCodeFromHexSeed(base::SysNSStringToUTF8(hexSeed)));
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
      
    return [UIImage imageWithCIImage:ciImage scale:[[UIScreen mainScreen] scale] orientation:UIImageOrientationUp];
  }
  return nil;
}

- (NSString *)getDeviceListJSON {
    auto deviceList = _worker->GetDeviceList();
    auto* localDeviecInfo = _worker->GetLocalDeviceInfo();

    base::Value deviceListValue(base::Value::Type::LIST);

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
