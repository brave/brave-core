/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_FAKE_SPARKLE_H_
#define BRAVE_BROWSER_MAC_FAKE_SPARKLE_H_

#import <Foundation/Foundation.h>

@class SparkleGlue;

// Stands in for Sparkle's `SUUpdateDriver`, recording install requests.
@interface FakeSUUpdateDriver : NSObject
@property(readonly) int installCount;
@property(readonly) BOOL lastRelaunch;
@property(readonly) BOOL lastShowUI;
- (void)installWithToolAndRelaunch:(BOOL)relaunch
           displayingUserInterface:(BOOL)showUI;
@end

// Stands in for Sparkle's `SUUpdater`; Sparkle is loaded at runtime, so its
// classes can't be subclassed in tests.
@interface FakeSUUpdater : NSObject
@property(strong) FakeSUUpdateDriver* driver;
@end

// Makes `glue` use `updater` and delivers Sparkle's callback for a downloaded
// update that will be installed when the browser quits.
void SimulateSparkleUpdateWillBeInstalledOnQuit(SparkleGlue* glue,
                                                FakeSUUpdater* updater);

#endif  // BRAVE_BROWSER_MAC_FAKE_SPARKLE_H_
