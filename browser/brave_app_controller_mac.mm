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

std::string GetDescriptionFromAppcastItem(SUAppcastItem* item) {
  NSString* description = [NSString stringWithFormat:@"AppcastItem(Date: %@, Version: %@)",
      [item dateString], [item versionString]];
  return [description UTF8String];
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
  [[SUUpdater sharedUpdater] setDelegate:(id<SUUpdaterDelegate>)self];
}

#pragma mark - SUUpdaterDelegate

- (void)updater:(SUUpdater *)updater
    didFinishLoadingAppcast:(SUAppcast *)appcast {
  VLOG(0) << "brave update: did finish loading appcast";
}

- (void)updater:(SUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)item {
  VLOG(0) << "brave update: did finish valid update with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)updater {
  VLOG(0) << "brave update: did not find update";
}

- (void)updater:(SUUpdater *)updater
    willDownloadUpdate:(SUAppcastItem *)item
           withRequest:(NSMutableURLRequest *)request {
  VLOG(0) << "brave update: willDownloadUpdate with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(SUUpdater *)updater
    failedToDownloadUpdate:(SUAppcastItem *)item error:(NSError *)error {
  VLOG(0) << "brave update: failed to download update with " +
             GetDescriptionFromAppcastItem(item) +
             " with error - " + [[error description] UTF8String];
}

- (void)userDidCancelDownload:(SUUpdater *)updater {
  VLOG(0) << "brave update: user did cancel download";
}

- (void)updater:(SUUpdater *)updater willInstallUpdate:(SUAppcastItem *)item {
  VLOG(0) << "brave update: will install update with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updaterWillRelaunchApplication:(SUUpdater *)updater {
  VLOG(0) << "brave update: will relaunch application";
}

- (void)updaterDidRelaunchApplication:(SUUpdater *)updater {
  VLOG(0) << "brave update: did relaunch application";
}

- (void)updater:(SUUpdater *)updater
    willInstallUpdateOnQuit:(SUAppcastItem *)item
    immediateInstallationInvocation:(NSInvocation *)invocation {
  VLOG(0) << "brave update: will install update on quit with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(SUUpdater *)updater
    didCancelInstallUpdateOnQuit:(SUAppcastItem *)item {
  VLOG(0) << "brave update: did cancel install update on quit with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(SUUpdater *)updater didAbortWithError:(NSError *)error {
  VLOG(0) << [[error description] UTF8String];
}

@end  // @implementation BraveAppController
