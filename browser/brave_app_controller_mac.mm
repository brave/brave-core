/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/brave_app_controller_mac.h"

#include <string>

#import "brave/browser/sparkle_glue_mac.h"
#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/common/brave_switches.h"

namespace {

BOOL UpdateEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableBraveUpdateTest);
}

std::string GetDescriptionFromAppcastItem(id item) {
  return [SparkleGlue descriptionFromAppcastItem:item];
}

}

@implementation BraveAppController
{
  SparkleGlue* sparkle_glue_;
}

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

  [sparkle_glue_ checkForUpdates:sender];
}

- (void)initializeBraveUpdater {
  DCHECK(UpdateEnabled());

  sparkle_glue_ = [SparkleGlue sharedSparkleGlue];
  [sparkle_glue_ setDelegate:self];
}

#pragma mark - SUUpdaterDelegate

- (void)updater:(id)updater didFinishLoadingAppcast:(id)appcast {
  VLOG(0) << "brave update: did finish loading appcast";
}

- (void)updater:(id)updater didFindValidUpdate:(id)item {
  VLOG(0) << "brave update: did finish valid update with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updaterDidNotFindUpdate:(id)updater {
  VLOG(0) << "brave update: did not find update";
}

- (void)updater:(id)updater
    willDownloadUpdate:(id)item
           withRequest:(NSMutableURLRequest *)request {
  VLOG(0) << "brave update: willDownloadUpdate with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(id)updater
    failedToDownloadUpdate:(id)item
                     error:(NSError *)error {
  VLOG(0) << "brave update: failed to download update with " +
             GetDescriptionFromAppcastItem(item) +
             " with error - " + [[error description] UTF8String];
}

- (void)userDidCancelDownload:(id)updater {
  VLOG(0) << "brave update: user did cancel download";
}

- (void)updater:(id)updater willInstallUpdate:(id)item {
  VLOG(0) << "brave update: will install update with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updaterWillRelaunchApplication:(id)updater {
  VLOG(0) << "brave update: will relaunch application";
}

- (void)updaterDidRelaunchApplication:(id)updater {
  VLOG(0) << "brave update: did relaunch application";
}

- (void)updater:(id)updater
    willInstallUpdateOnQuit:(id)item
    immediateInstallationInvocation:(NSInvocation *)invocation {
  VLOG(0) << "brave update: will install update on quit with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(id)updater didCancelInstallUpdateOnQuit:(id)item {
  VLOG(0) << "brave update: did cancel install update on quit with " +
             GetDescriptionFromAppcastItem(item);
}

- (void)updater:(id)updater didAbortWithError:(NSError *)error {
  VLOG(0) << [[error description] UTF8String];
}

@end  // @implementation BraveAppController
