/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
#define BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_

#import <Foundation/Foundation.h>

@class BraveBookmarksAPI;
@class BraveHistoryAPI;
@class BravePasswordAPI;
@class BraveOpenTabsAPI;
@class BraveSendTabAPI;
@class BraveSyncAPI;
@class BraveSyncProfileServiceIOS;
@class BraveStats;
@class BraveWalletAPI;
@class AdblockService;
@class BraveTabGeneratorAPI;

NS_ASSUME_NONNULL_BEGIN

typedef NSString* BraveCoreSwitch NS_STRING_ENUM;
/// Overrides the component updater source. Defaults to the CI provided value
///
/// Expected value: url-source={url}
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchComponentUpdater;
/// Overrides Chromium VLOG verbosity. Defaults to only printing from folders
/// existing within a `brave` subfolder up to level 5.
///
/// Expected value: {folder-expression}={level}
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchVModule;
/// Overrides the sync service base URL. Defaults to the CI provided value
///
/// Expected value: A URL string
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchSyncURL;
/// Sets a number of overrides for the ads & ledger services such as which
/// environment its using, debug mode, etc.
///
/// Expected value: A comma-separated list of flags, including:
///     - staging={bool}
////    - development={bool}
///     - debug={bool}
///     - reconcile-interval={int}
///     - retry-interval={int}
OBJC_EXPORT const BraveCoreSwitch BraveCoreSwitchRewardsFlags;

typedef int BraveCoreLogSeverity NS_TYPED_ENUM;
OBJC_EXPORT const BraveCoreLogSeverity BraveCoreLogSeverityFatal;
OBJC_EXPORT const BraveCoreLogSeverity BraveCoreLogSeverityError;
OBJC_EXPORT const BraveCoreLogSeverity BraveCoreLogSeverityWarning;
OBJC_EXPORT const BraveCoreLogSeverity BraveCoreLogSeverityInfo;
OBJC_EXPORT const BraveCoreLogSeverity BraveCoreLogSeverityVerbose;

typedef bool (^BraveCoreLogHandler)(BraveCoreLogSeverity severity,
                                    NSString* file,
                                    int line,
                                    size_t messageStart,
                                    NSString* formattedMessage);

OBJC_EXPORT
@interface BraveCoreMain : NSObject

@property(nonatomic, readonly) BraveBookmarksAPI* bookmarksAPI;

@property(nonatomic, readonly) BraveHistoryAPI* historyAPI;

@property(nonatomic, readonly) BravePasswordAPI* passwordAPI;

@property(nonatomic, readonly) BraveOpenTabsAPI* openTabsAPI;

@property(nonatomic, readonly) BraveSendTabAPI* sendTabAPI;

@property(nonatomic, readonly) BraveSyncAPI* syncAPI;

@property(nonatomic, readonly) BraveSyncProfileServiceIOS* syncProfileService;

@property(nonatomic, readonly) BraveTabGeneratorAPI* tabGeneratorAPI;

/// Sets the global log handler for Chromium & BraveCore logs.
///
/// When a custom log handler is set, it is the responsibility of the client
/// to handle fatal logs from CHECK (and DCHECK on debug builds) by checking
/// the `serverity` passed in.
+ (void)setLogHandler:(nullable BraveCoreLogHandler)logHandler;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithUserAgent:(NSString*)userAgent;

- (instancetype)initWithUserAgent:(NSString*)userAgent
               additionalSwitches:(NSDictionary<BraveCoreSwitch, NSString*>*)
                                      additionalSwitches;

- (void)scheduleLowPriorityStartupTasks;

@property(readonly) BraveWalletAPI* braveWalletAPI;

@property(readonly) BraveStats* braveStats;

@property(readonly) AdblockService* adblockService;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
