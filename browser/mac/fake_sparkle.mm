/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/fake_sparkle.h"

#import "brave/browser/mac/sparkle_glue.h"

// SparkleGlue implements this `SUUpdaterDelegate` method.
@interface SparkleGlue (SUUpdaterDelegateForTesting)
- (void)updater:(id)updater
            willInstallUpdateOnQuit:(id)item
    immediateInstallationInvocation:(NSInvocation*)invocation;
@end

// Stands in for Sparkle's `SUAppcastItem`.
@interface FakeSUAppcastItem : NSObject
- (NSString*)dateString;
- (NSString*)displayVersionString;
@end

@implementation FakeSUAppcastItem
- (NSString*)dateString {
  return @"Thu, 29 Oct 2026 00:00:00 +0000";
}
- (NSString*)displayVersionString {
  return @"999.0.0.0";
}
@end

@implementation FakeSUUpdateDriver
@synthesize installCount = _installCount;
@synthesize lastRelaunch = _lastRelaunch;
@synthesize lastShowUI = _lastShowUI;

- (void)installWithToolAndRelaunch:(BOOL)relaunch
           displayingUserInterface:(BOOL)showUI {
  ++_installCount;
  _lastRelaunch = relaunch;
  _lastShowUI = showUI;
}
@end

@implementation FakeSUUpdater
@synthesize driver = _driver;
@end

void SimulateSparkleUpdateWillBeInstalledOnQuit(SparkleGlue* glue,
                                                FakeSUUpdater* updater) {
  [glue setUpdaterForTesting:updater];
  [glue updater:updater
              willInstallUpdateOnQuit:[[FakeSUAppcastItem alloc] init]
      immediateInstallationInvocation:nil];
}
