/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/brave_app_controller_mac.h"

#include <string>

#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/common/brave_switches.h"
#import <Sparkle/Sparkle.h>

@implementation BraveAppController

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  [super applicationDidFinishLaunching:notify];

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableBraveUpdateTest))
    [self startBraveUpdater];
}

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  [super applicationWillFinishLaunching:notification];

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableBraveUpdateTest))
    [self initializeBraveUpdater];
}

- (void)initializeBraveUpdater {
  constexpr int kBraveUpdateCheckInterval = 60;

  SUUpdater* updater = [SUUpdater sharedUpdater];
  [updater setAutomaticallyChecksForUpdates:YES];
  [updater setAutomaticallyDownloadsUpdates:YES];
  [updater setUpdateCheckInterval:kBraveUpdateCheckInterval];
  [updater setDelegate:(id<SUUpdaterDelegate>)self];

  // Reset previous feed url set by setFeedURL().
  // In the SUUpdater document, that value persists in the host bundle's user
  // default.
  // Tried to remove that value by removing Brave.app bundle in out/Release and
  // ~/Library/Preferences/org.brave.Brave.plist.
  // After these removing, previous value(https://example.org) is still remained
  // and used when there is SUFeedURL is exsited in Info.plist.
  // When nil is set, SUFeedURL value is used.
  // TODO(shong): Remove this.
  [updater setFeedURL:nil];
}

- (void)startBraveUpdater {
  [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)updater {
}

@end  // @implementation BraveAppController
