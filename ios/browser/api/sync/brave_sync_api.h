/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class BraveSyncInternalsController;

typedef NSInteger BraveSyncAPISyncProtocolErrorResult
    NS_TYPED_ENUM NS_SWIFT_NAME(BraveSyncAPI.SyncProtocolErrorResult);

OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultSuccess;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultNotMyBirthday;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultThrottled;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultTransientError;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultMigrationDone;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultDisabledByAdmin;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultPartialFailure;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultDataObsolete;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultEncryptionObsolete;
OBJC_EXPORT BraveSyncAPISyncProtocolErrorResult const
    BraveSyncAPISyncProtocolErrorResultUnknown;

typedef NSInteger BraveSyncAPIQrCodeDataValidationResult NS_TYPED_ENUM
    NS_SWIFT_NAME(BraveSyncAPI.QrCodeDataValidationResult);

OBJC_EXPORT BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultValid;
OBJC_EXPORT BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultNotWellFormed;
OBJC_EXPORT BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultVersionDeprecated;
OBJC_EXPORT BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultExpired;
OBJC_EXPORT BraveSyncAPIQrCodeDataValidationResult const
    BraveSyncAPIQrCodeDataValidationResultValidForTooLong;

typedef NSInteger BraveSyncAPIWordsValidationStatus NS_TYPED_ENUM
    NS_SWIFT_NAME(BraveSyncAPI.WordsValidationStatus);

OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusValid;
OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusNotValidPureWords;
OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusVersionDeprecated;
OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusExpired;
OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusValidForTooLong;
OBJC_EXPORT BraveSyncAPIWordsValidationStatus const
    BraveSyncAPIWordsValidationStatusWrongWordsNumber;

OBJC_EXPORT
@interface BraveSyncAPI : NSObject

@property(nonatomic, readonly) bool canSyncFeatureStart;
@property(nonatomic, readonly) bool isSyncFeatureActive;
@property(nonatomic, readonly) bool isInitialSyncFeatureSetupComplete;
@property(nonatomic) bool isSyncAccountDeletedNoticePending;
@property(nonatomic, readonly) bool isFailedDecryptSeedNoticeDismissed;

- (instancetype)init NS_UNAVAILABLE;

- (void)requestSync;

- (void)setSetupComplete;

- (void)resetSync;

- (void)setDidJoinSyncChain:(void (^)(bool))completion;

- (void)permanentlyDeleteAccount:
    (void (^)(BraveSyncAPISyncProtocolErrorResult))completion;

- (void)deleteDevice:(NSString*)guid;

- (bool)isValidSyncCode:(NSString*)syncCode;

- (NSString*)getSyncCode;

// returns false is sync is already configured or if the sync code is invalid
- (bool)setSyncCode:(NSString*)syncCode;

- (NSString*)syncCodeFromHexSeed:(NSString*)hexSeed;

- (NSString*)hexSeedFromSyncCode:(NSString*)syncCode;

- (NSString*)qrCodeJsonFromHexSeed:(NSString*)hexSeed;

- (void)dismissFailedDecryptSeedNotice;

- (BraveSyncAPIQrCodeDataValidationResult)getQRCodeValidationResult:
    (NSString*)json;

- (BraveSyncAPIWordsValidationStatus)getWordsValidationResult:
    (NSString*)timeLimitedWords;

- (NSString*)getWordsFromTimeLimitedWords:(NSString*)timeLimitedWords;

- (NSString*)getTimeLimitedWordsFromWords:(NSString*)words;

- (NSString*)getHexSeedFromQrCodeJson:(NSString*)json;

- (nullable UIImage*)getQRCodeImage:(CGSize)size;

- (nullable NSString*)getDeviceListJSON;

- (id)createSyncDeviceObserver:(void (^)())onDeviceInfoChanged;
- (id)createSyncServiceObserver:(void (^)())onSyncServiceStateChanged
          onSyncServiceShutdown:(void (^)())onSyncServiceShutdown;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_API_H_
