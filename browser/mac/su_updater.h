/* This Source Code Form is subject to the terms of the Mozilla Public
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

@property (strong) SUUpdateDriver *driver;

+ (SUUpdater *)sharedUpdater;

- (void)checkForUpdates:(id)sender;
- (void)setDelegate:(id)delegate;
- (void)setAutomaticallyChecksForUpdates:(BOOL)enable;
- (void)setAutomaticallyDownloadsUpdates:(BOOL)enable;
- (void)checkForUpdatesInBackground;
- (void)setUpdateCheckInterval:(NSTimeInterval)interval;

@end

#endif  // BRAVE_BROWSER_MAC_SU_UPDATER_H_
