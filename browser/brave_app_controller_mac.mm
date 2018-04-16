/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/brave_app_controller_mac.h"

#include <string>

#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/common/brave_switches.h"
#import <Sparkle/Sparkle.h>

namespace {

BOOL UpdateEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableBraveUpdateTest);
}

}

@implementation BraveAppController

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  [super applicationWillFinishLaunching:notification];

  if (!UpdateEnabled())
    return;

  [self initializeBraveUpdater];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  if ([super validateUserInterfaceItem:item])
    return YES;

  return  [item action] == @selector(updateBrave:) ? UpdateEnabled() : NO;
}

- (IBAction)updateBrave:(id)sender {
  DCHECK(UpdateEnabled());

  [[SUUpdater sharedUpdater] checkForUpdates:sender];
}

- (void)initializeBraveUpdater {
  SUUpdater* updater = [SUUpdater sharedUpdater];

  [updater setDelegate:(id<SUUpdaterDelegate>)self];

  // Reset previous feed url set by setFeedURL().
  // In the SUUpdater document, that value persists in the host bundle's user
  // default.
  // Tried to remove that value by removing Brave.app bundle in out/Release and
  // ~/Library/Preferences/org.brave.Brave.plist.
  // After these removing, previous value(https://example.org) is still remained
  // and used when there is SUFeedURL is exsited in Info.plist.
  // When nil is set, SUFeedURL value in Info.plist is used.
  // TODO(shong): Remove this.
  [updater setFeedURL:nil];
}

#pragma mark - SUUpdaterDelegate

- (void)updater:(SUUpdater *)updater
    didFinishLoadingAppcast:(SUAppcast *)appcast {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)item {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater
    willDownloadUpdate:(SUAppcastItem *)item
           withRequest:(NSMutableURLRequest *)request {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater
    failedToDownloadUpdate:(SUAppcastItem *)item error:(NSError *)error {
  LOG(ERROR) << __FUNCTION__;
}

- (void)userDidCancelDownload:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)item {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updaterWillRelaunchApplication:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updaterDidRelaunchApplication:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updaterWillShowModalAlert:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updaterDidShowModalAlert:(SUUpdater *)updater {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater
    willInstallUpdateOnQuit:(SUAppcastItem *)item
    immediateInstallationInvocation:(NSInvocation *)invocation {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater
    didCancelInstallUpdateOnQuit:(SUAppcastItem *)item {
  LOG(ERROR) << __FUNCTION__;
}

- (void)updater:(SUUpdater *)updater didAbortWithError:(NSError *)error {
  LOG(ERROR) << [[error description] UTF8String];
}

@end  // @implementation BraveAppController
