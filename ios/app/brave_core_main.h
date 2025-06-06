/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
#define BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_

#import <Foundation/Foundation.h>

#import "brave_core_switches.h"  // NOLINT

@class BraveP3AUtils;
@class AdblockService;
@class HTTPSUpgradeExceptionsService;
@class BraveUserAgentExceptionsIOS;
@class BraveProfileController;

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


@property(nonatomic, readonly)
    HTTPSUpgradeExceptionsService* httpsUpgradeExceptionsService;

@property(nonatomic, readonly, nullable)
    BraveUserAgentExceptionsIOS* braveUserAgentExceptions;

/// Sets the global log handler for Chromium & BraveCore logs.
///
/// When a custom log handler is set, it is the responsibility of the client
/// to handle fatal logs from CHECK (and DCHECK on debug builds) by checking
/// the `serverity` passed in.
+ (void)setLogHandler:(nullable BraveCoreLogHandler)logHandler;

- (instancetype)init;

- (instancetype)initWithAdditionalSwitches:
    (NSArray<BraveCoreSwitch*>*)additionalSwitches;

- (void)scheduleLowPriorityStartupTasks;

- (void)setUserAgent:(NSString*)userAgent;

@property(readonly) AdblockService* adblockService;

- (void)initializeP3AServiceForChannel:(NSString*)channel
                      installationDate:(NSDate*)installDate;

@property(readonly) BraveP3AUtils* p3aUtils;

@property(readonly, nullable) BraveProfileController* profileController;
- (void)loadDefaultProfile:(void (^)(BraveProfileController*))completionHandler;

/// Sets up bundle path overrides and initializes ICU from the BraveCore bundle
/// without setting up a BraveCoreMain instance.
///
/// Should only be called in unit tests
+ (bool)initializeICUForTesting;

+ (void)initializeResourceBundleForTesting;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_APP_BRAVE_CORE_MAIN_H_
