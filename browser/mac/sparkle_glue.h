/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
#define BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_

#import "brave/browser/mac/keystone_glue.h"

#if defined(__OBJC__)

#import <Foundation/Foundation.h>

@interface SparkleGlue : NSObject

+ (instancetype)sharedSparkleGlue;

- (instancetype)init;

- (void)registerWithSparkle;

- (void)checkForUpdates;

- (BOOL)relaunch;

- (AutoupdateStatus)recentStatus;

- (NSNotification*)recentNotification;  // NOLINT - not sure how to fix this

// Returns YES if an asynchronous operation is pending: if an update check or
// installation attempt is currently in progress.
- (BOOL)asyncOperationPending;

// Returns YES if the application is running from a read-only filesystem,
// such as a disk image.
- (BOOL)isOnReadOnlyFilesystem;

// Makes `sharedSparkleGlue` return `glue`, which may be nil.
+ (void)setSharedSparkleGlueForTesting:(SparkleGlue*)glue;

// Uses `updater` in place of Sparkle's `SUUpdater`.
- (void)setUpdaterForTesting:(id)updater;

@end  // @interface SparkleGlue

#endif  // __OBJC__

namespace sparkle_glue {

bool SparkleEnabled();

std::u16string CurrentlyInstalledVersion();

}  // namespace sparkle_glue

#endif  // BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
