/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/sync/brave_sync_api.h"

#import <CoreImage/CoreImage.h>

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_sync/crypto/crypto.h"
#include "brave/components/brave_sync/qr_code_validator.h"
#include "brave/components/brave_sync/time_limited_words.h"
#include "brave/components/sync_device_info/brave_device_info.h"
#include "brave/ios/browser/api/sync/brave_sync_worker.h"
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

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - BraveSyncAPISyncProtocolErrorResult

BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultSuccess =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::SYNC_SUCCESS);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultNotMyBirthday =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::NOT_MY_BIRTHDAY);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultThrottled =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::THROTTLED);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultTransientError =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::TRANSIENT_ERROR);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultMigrationDone =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::MIGRATION_DONE);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultDisabledByAdmin = static_cast<NSInteger>(
        syncer::SyncProtocolErrorType::DISABLED_BY_ADMIN);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultPartialFailure =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::PARTIAL_FAILURE);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultDataObsolete = static_cast<NSInteger>(
        syncer::SyncProtocolErrorType::CLIENT_DATA_OBSOLETE);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultEncryptionObsolete =
        static_cast<NSInteger>(
            syncer::SyncProtocolErrorType::ENCRYPTION_OBSOLETE);
BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultUnknown =
        static_cast<NSInteger>(syncer::SyncProtocolErrorType::UNKNOWN_ERROR);

// MARK: - QrCodeDataValidationResult

BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultValid =
        static_cast<NSInteger>(brave_sync::QrCodeDataValidationResult::kValid);
BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultNotWellFormed =
        static_cast<NSInteger>(
            brave_sync::QrCodeDataValidationResult::kNotWellFormed);
BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultVersionDeprecated =
        static_cast<NSInteger>(
            brave_sync::QrCodeDataValidationResult::kVersionDeprecated);
BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultExpired = static_cast<NSInteger>(
        brave_sync::QrCodeDataValidationResult::kExpired);
BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultValidForTooLong =
        static_cast<NSInteger>(
            brave_sync::QrCodeDataValidationResult::kValidForTooLong);

// MARK: - TimeLimitedWords::ValidationStatus

BraveSyncAPIWordsValidationStatus const BraveSyncAPIWordsValidationStatusValid =
    static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kValid);
BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusNotValidPureWords = static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kNotValidPureWords);
BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusVersionDeprecated = static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kVersionDeprecated);
BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusExpired = static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kExpired);
BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusValidForTooLong = static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kValidForTooLong);
BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusWrongWordsNumber = static_cast<NSInteger>(
        brave_sync::TimeLimitedWords::ValidationStatus::kWrongWordsNumber);

// MARK: - BraveSyncDeviceObserver

@interface BraveSyncDeviceObserver : NSObject {
  std::unique_ptr<BraveSyncDeviceTracker> _device_observer;
}
@end

@implementation BraveSyncDeviceObserver

- (instancetype)initWithDeviceInfoTracker:(syncer::DeviceInfoTracker*)tracker
                                 callback:(void (^)())onDeviceInfoChanged {
  if ((self = [super init])) {
    _device_observer = std::make_unique<BraveSyncDeviceTracker>(
        tracker, base::BindRepeating(onDeviceInfoChanged));
  }
  return self;
}
@end

// MARK: - BraveSyncServiceObserver

@interface BraveSyncServiceObserver : NSObject {
  std::unique_ptr<BraveSyncServiceTracker> _service_tracker;
}
@end

@implementation BraveSyncServiceObserver

- (instancetype)initWithSyncServiceImpl:
                    (syncer::SyncServiceImpl*)syncServiceImpl
                   stateChangedCallback:(void (^)())onSyncServiceStateChanged
                   syncShutdownCallback:(void (^)())onSyncServiceShutdown {
  if ((self = [super init])) {
    _service_tracker = std::make_unique<BraveSyncServiceTracker>(
        syncServiceImpl, base::BindRepeating(onSyncServiceStateChanged),
        base::BindRepeating(onSyncServiceShutdown));
  }
  return self;
}
@end

// MARK: - BraveSyncAPI

@interface BraveSyncAPI () {
  std::unique_ptr<BraveSyncWorker> _worker;
  raw_ptr<ChromeBrowserState> _chromeBrowserState;
}
@end

@implementation BraveSyncAPI

- (instancetype)initWithBrowserState:(ChromeBrowserState*)mainBrowserState {
  if ((self = [super init])) {
    _chromeBrowserState = mainBrowserState;
    _worker.reset(new BraveSyncWorker(_chromeBrowserState));
  }
  return self;
}

- (void)dealloc {
  _worker.reset();
  _chromeBrowserState = nullptr;
}

- (bool)canSyncFeatureStart {
  return _worker->CanSyncFeatureStart();
}

- (bool)isSyncFeatureActive {
  return _worker->IsSyncFeatureActive();
}

- (bool)isInitialSyncFeatureSetupComplete {
  return _worker->IsInitialSyncFeatureSetupComplete();
}

- (void)setSetupComplete {
  _worker->SetSetupComplete();
}

- (void)requestSync {
  _worker->RequestSync();
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

- (NSString*)hexSeedFromSyncCode:(NSString*)syncCode {
  return base::SysUTF8ToNSString(
      _worker->GetHexSeedFromSyncCode(base::SysNSStringToUTF8(syncCode)));
}

- (NSString*)qrCodeJsonFromHexSeed:(NSString*)hexSeed {
  return base::SysUTF8ToNSString(
      _worker->GetQrCodeJsonFromHexSeed(base::SysNSStringToUTF8(hexSeed)));
}

- (BraveSyncAPIQrCodeDataValidationResult)getQRCodeValidationResult:
    (NSString*)json {
  return static_cast<BraveSyncAPIQrCodeDataValidationResult>(
      _worker->GetQrCodeValidationResult(base::SysNSStringToUTF8(json)));
}

- (BraveSyncAPIWordsValidationStatus)getWordsValidationResult:
    (NSString*)timeLimitedWords {
  return static_cast<BraveSyncAPIWordsValidationStatus>(
      _worker->GetWordsValidationResult(
          base::SysNSStringToUTF8(timeLimitedWords)));
}

- (NSString*)getWordsFromTimeLimitedWords:(NSString*)timeLimitedWords {
  return base::SysUTF8ToNSString(_worker->GetWordsFromTimeLimitedWords(
      base::SysNSStringToUTF8(timeLimitedWords)));
}

- (NSString*)getTimeLimitedWordsFromWords:(NSString*)words {
  return base::SysUTF8ToNSString(
      _worker->GetTimeLimitedWordsFromWords(base::SysNSStringToUTF8(words)));
}

- (NSDate*)getExpirationFromTimeLimitedWords:(NSString*)timeLimitedWords {
  base::Time not_after = brave_sync::TimeLimitedWords::GetNotAfter(
      base::SysNSStringToUTF8(timeLimitedWords));

  return not_after.ToNSDate();
}

- (NSString*)getHexSeedFromQrCodeJson:(NSString*)json {
  return base::SysUTF8ToNSString(
      _worker->GetHexSeedFromQrCodeJson(base::SysNSStringToUTF8(json)));
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

  base::Value::List device_list_value;

  for (const auto& device : device_list) {
    auto device_value = device->ToValue();
    bool is_current_device =
        local_device_info ? local_device_info->guid() == device->guid() : false;
    device_value.Set("isCurrentDevice", is_current_device);
    device_value.Set("guid", device->guid());
    device_value.Set("supportsSelfDelete", device->is_self_delete_supported());
    device_list_value.Append(base::Value(std::move(device_value)));
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

- (void)setDidJoinSyncChain:(void (^)(bool success))completion {
  _worker->SetJoinSyncChainCallback(base::BindOnce(completion));
}

- (void)permanentlyDeleteAccount:
    (void (^)(BraveSyncAPISyncProtocolErrorResult))completion {
  _worker->PermanentlyDeleteAccount(base::BindOnce(
      [](void (^completion)(BraveSyncAPISyncProtocolErrorResult),
         const syncer::SyncProtocolError& error) {
        completion(
            static_cast<BraveSyncAPISyncProtocolErrorResult>(error.error_type));
      },
      completion));
}

- (bool)isSyncAccountDeletedNoticePending {
  brave_sync::Prefs brave_sync_prefs(_chromeBrowserState->GetPrefs());
  return brave_sync_prefs.IsSyncAccountDeletedNoticePending();
}

- (void)setIsSyncAccountDeletedNoticePending:
    (bool)isSyncAccountDeletedNoticePending {
  brave_sync::Prefs brave_sync_prefs(_chromeBrowserState->GetPrefs());
  brave_sync_prefs.SetSyncAccountDeletedNoticePending(false);
}

- (bool)isFailedDecryptSeedNoticeDismissed {
  brave_sync::Prefs brave_sync_prefs(_chromeBrowserState->GetPrefs());
  return brave_sync_prefs.IsFailedDecryptSeedNoticeDismissed();
}

- (void)dismissFailedDecryptSeedNotice {
  brave_sync::Prefs brave_sync_prefs(_chromeBrowserState->GetPrefs());
  brave_sync_prefs.DismissFailedDecryptSeedNotice();
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

- (id)createSyncServiceObserver:(void (^)())onSyncServiceStateChanged
          onSyncServiceShutdown:(void (^)())onSyncServiceShutdown {
  auto* service = static_cast<syncer::SyncServiceImpl*>(
      SyncServiceFactory::GetForBrowserState(_chromeBrowserState));
  return [[BraveSyncServiceObserver alloc]
      initWithSyncServiceImpl:service
         stateChangedCallback:onSyncServiceStateChanged
         syncShutdownCallback:onSyncServiceShutdown];
}
@end
