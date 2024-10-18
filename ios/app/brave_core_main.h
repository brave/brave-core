/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
#define BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_

#import <Foundation/Foundation.h>

#import "brave_core_switches.h"  // NOLINT

@class BraveBookmarksAPI;
@class BraveHistoryAPI;
@class BravePasswordAPI;
@class BraveOpenTabsAPI;
@class BraveP3AUtils;
@class BraveSendTabAPI;
@class BraveSyncAPI;
@class BraveSyncProfileServiceIOS;
@class BraveStats;
@class BraveWalletAPI;
@class AdblockService;
@class BraveTabGeneratorAPI;
@class WebImageDownloader;
@class NTPBackgroundImagesService;
@class DeAmpPrefs;
@class AIChat;
@class DefaultHostContentSettings;
@class BrowserPrefs;
@class CWVWebViewConfiguration;
@protocol AIChatDelegate;
@protocol IpfsAPI;

NS_ASSUME_NONNULL_BEGIN

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

@property(nonatomic, readonly) WebImageDownloader* webImageDownloader;

/// Sets the global log handler for Chromium & BraveCore logs.
///
/// When a custom log handler is set, it is the responsibility of the client
/// to handle fatal logs from CHECK (and DCHECK on debug builds) by checking
/// the `serverity` passed in.
+ (void)setLogHandler:(nullable BraveCoreLogHandler)logHandler;

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithUserAgent:(NSString*)userAgent;

- (instancetype)initWithUserAgent:(NSString*)userAgent
               additionalSwitches:
                   (NSArray<BraveCoreSwitch*>*)additionalSwitches;

- (void)scheduleLowPriorityStartupTasks;

@property(readonly) BraveWalletAPI* braveWalletAPI;

@property(readonly) BraveStats* braveStats;

@property(readonly) AdblockService* adblockService;

@property(readonly) id<IpfsAPI> ipfsAPI;

- (void)initializeP3AServiceForChannel:(NSString*)channel
                         weekOfInstall:(NSString*)weekOfInstall;

@property(readonly) BraveP3AUtils* p3aUtils;

@property(readonly) DeAmpPrefs* deAmpPrefs;

@property(readonly) BrowserPrefs* browserPrefs;

@property(readonly) NTPBackgroundImagesService* backgroundImagesService;

/// The default content settings for regular browsing windows
@property(readonly) DefaultHostContentSettings* defaultHostContentSettings;

- (AIChat*)aiChatAPIWithDelegate:(id<AIChatDelegate>)delegate;

/// Sets up bundle path overrides and initializes ICU from the BraveCore bundle
/// without setting up a BraveCoreMain instance.
///
/// Should only be called in unit tests
+ (bool)initializeICUForTesting;

@property(readonly) CWVWebViewConfiguration* defaultWebViewConfiguration;
@property(readonly) CWVWebViewConfiguration* nonPersistentWebViewConfiguration;
- (void)notifyLastPrivateTabClosed;

@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
