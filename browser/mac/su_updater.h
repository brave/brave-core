/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_SU_UPDATER_H_
#define BRAVE_BROWSER_MAC_SU_UPDATER_H_

#import <Foundation/Foundation.h>

@interface SUUpdateDriver;

- (void)installWithToolAndRelaunch:(BOOL)relaunch displayingUserInterface
                                  :(BOOL)showUI;

@end

@interface SUUpdater : NSObject

@property(strong) SUUpdateDriver* driver;  // NOLINT

+ (SUUpdater*)sharedUpdater;  // NOLINT

- (void)checkForUpdates:(id)sender;
- (void)setDelegate:(id)delegate;
- (void)setAutomaticallyChecksForUpdates:(BOOL)enable;
- (void)setAutomaticallyDownloadsUpdates:(BOOL)enable;
- (void)checkForUpdatesInBackground;
- (void)checkForUpdatesInBackgroundWithoutUi;
- (void)setUpdateCheckInterval:(NSTimeInterval)interval;

@property BOOL automaticallyDownloadsUpdates;

@end

#endif  // BRAVE_BROWSER_MAC_SU_UPDATER_H_
