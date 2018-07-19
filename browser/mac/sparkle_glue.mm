/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/sparkle_glue.h"

#include <string>
#include <sys/mount.h>
#include <sys/stat.h>

#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "brave/browser/mac/su_updater.h"
#include "brave/browser/update_util.h"
#include "chrome/common/channel_info.h"

namespace {

std::string GetDescriptionFromAppcastItem(id item) {
  NSString* description =
      [NSString stringWithFormat:@"AppcastItem(Date: %@, Version: %@)",
          [item performSelector:@selector(dateString)],
          [item performSelector:@selector(versionString)]];
  return [description UTF8String];
}

}  // namespace

@implementation SparkleGlue
{
  SUUpdater* su_updater_;
  BOOL registered_;
  NSString* appPath_;
  NSString* url_;
  std::string channel_;
}

#pragma mark - SparkleGlue

+ (instancetype)sharedSparkleGlue {
  static SparkleGlue* shared;
  if (brave::UpdateEnabled() && shared == nil) {
    shared = [[SparkleGlue alloc] init];
    [shared loadParameters];
    if (![shared loadSparkleFramework]) {
      VLOG(0) << "brave update: Failed to load sparkle framework";
      [shared release];
      shared = nil;
    }
  }
  return shared;
}

- (instancetype)init {
  if (self = [super init]) {
    registered_ = false;
    su_updater_ = nil;
    return self;
  } else {
    return nil;
  }
}

- (void)dealloc {
  [appPath_ release];
  [url_ release];

  [super dealloc];
}

- (BOOL)loadSparkleFramework {
  if (!appPath_ || !url_)
    return NO;

  if ([self isOnReadOnlyFilesystem])
    return NO;

  NSString* sparkle_path =
      [[base::mac::FrameworkBundle() privateFrameworksPath]
          stringByAppendingPathComponent:@"Sparkle.framework"];
  DCHECK(sparkle_path);

  NSBundle* sparkle_bundle = [NSBundle bundleWithPath:sparkle_path];
  [sparkle_bundle load];

  registered_ = false;

  Class sparkle_class = [sparkle_bundle classNamed:@"SUUpdater"];
  su_updater_ = [sparkle_class sharedUpdater];

  return YES;
}

- (void)registerWithSparkle {
  DCHECK(brave::UpdateEnabled());
  DCHECK(su_updater_);

  registered_ = true;

  [su_updater_ setDelegate:self];
  [su_updater_ setAutomaticallyChecksForUpdates:YES];
  [su_updater_ setAutomaticallyDownloadsUpdates:YES];

  // Background update check interval.
  constexpr int kBraveUpdateCheckIntervalInSec = 60 * 60;
  [su_updater_ setUpdateCheckInterval:kBraveUpdateCheckIntervalInSec];

  // Start background update.
  [su_updater_ checkForUpdatesInBackground];
}

- (void)checkForUpdates {
  DCHECK(registered_);
  [su_updater_ checkForUpdates:nil];
}

- (void)checkForUpdatesInBackground {
  DCHECK(registered_);
  [su_updater_ checkForUpdatesInBackground];
}

- (BOOL)isOnReadOnlyFilesystem {
  const char* appPathC = [appPath_ fileSystemRepresentation];
  struct statfs statfsBuf;

  if (statfs(appPathC, &statfsBuf) != 0) {
    PLOG(ERROR) << "statfs";
    // Be optimistic about the filesystem's writability.
    return NO;
  }

  return (statfsBuf.f_flags & MNT_RDONLY) != 0;
}

- (void)loadParameters {
  NSBundle* appBundle = base::mac::OuterBundle();
  NSDictionary* infoDictionary = [self infoDictionary];

  NSString* appPath = [appBundle bundlePath];
  NSString* url = base::mac::ObjCCast<NSString>(
      [infoDictionary objectForKey:@"SUFeedURL"]);

  if (!appPath || !url) {
    // If parameters required for sparkle are missing, don't use it.
    return;
  }

  channel_ = chrome::GetChannelName();
  appPath_ = [appPath retain];
  url_ = [url retain];
}

- (NSDictionary*)infoDictionary {
  // Use base::mac::OuterBundle() to get the Chrome app's own bundle identifier
  // and path, not the framework's.  For auto-update, the application is
  // what's significant here: it's used to locate the outermost part of the
  // application for the existence checker and other operations that need to
  // see the entire application bundle.
  return [base::mac::OuterBundle() infoDictionary];
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
@end
