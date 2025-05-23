/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/sparkle_glue.h"

#include <string>
#include <sys/mount.h>
#include <sys/stat.h>

#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#import "base/apple/scoped_nsautorelease_pool.h"
#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/strings/sys_string_conversions.h"
#include "base/system/sys_info.h"
#include "base/task/thread_pool.h"
#import "brave/browser/mac/su_updater.h"
#include "brave/browser/update_util.h"
#include "brave/common/brave_channel_info.h"
#include "brave/components/constants/brave_switches.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

std::string GetUpdateChannel() {
  std::string channel_name = brave::GetChannelName();
  if (channel_name == "release")
    channel_name = "stable";
  return base::SysInfo::OperatingSystemArchitecture() == "x86_64"
             ? channel_name
             : channel_name + "-arm64";
}

NSString* GetVersionFromAppcastItem(id item) {
  return [item performSelector:@selector(displayVersionString)];
}

std::string GetDescriptionFromAppcastItem(id item) {
  NSString* description =
      [NSString stringWithFormat:@"AppcastItem(Date: %@, Version: %@)",
          [item performSelector:@selector(dateString)],
          GetVersionFromAppcastItem(item)];
  return base::SysNSStringToUTF8(description);
}

}  // namespace

@implementation SparkleGlue
{
  SUUpdater* __strong _su_updater;

  BOOL _registered;
  BOOL _updateWillBeInstalledOnQuit;

  NSString* __strong _appPath;
  NSString* __strong _new_version;

  // The most recent kAutoupdateStatusNotification notification posted.
  NSNotification* __strong _recentNotification;
}

#pragma mark - SparkleGlue

+ (instancetype)sharedSparkleGlue {
  static bool sTriedCreatingSharedSparkleGlue = false;
  static SparkleGlue* shared = nil;

  if (brave::UpdateEnabled() && !sTriedCreatingSharedSparkleGlue) {
    sTriedCreatingSharedSparkleGlue = true;

    shared = [[SparkleGlue alloc] init];
    [shared loadParameters];
    if (![shared loadSparkleFramework]) {
      VLOG(0) << "brave update: Failed to load sparkle framework";
      shared = nil;
    }
  }
  return shared;
}

- (instancetype)init {
  if (self = [super init]) {
    _registered = false;
    _su_updater = nil;
    return self;
  } else {
    return nil;
  }
}

- (BOOL)loadSparkleFramework {
  if (!_appPath) {
    return NO;
  }

  if ([self isOnReadOnlyFilesystem])
    return NO;

  DCHECK(!_su_updater);

  NSString* sparkle_path =
      [[base::apple::FrameworkBundle() privateFrameworksPath]
          stringByAppendingPathComponent:@"Sparkle.framework"];
  DCHECK(sparkle_path);

  NSBundle* sparkle_bundle = [NSBundle bundleWithPath:sparkle_path];
  [sparkle_bundle load];

  _registered = false;

  Class sparkle_class = [sparkle_bundle classNamed:@"SUUpdater"];
  _su_updater = [sparkle_class sharedUpdater];

  return YES;
}

- (void)registerWithSparkle {
  // This can be called by BraveBrowserMainPartsMac::PreMainMessageLoopStart()
  // again when browser is relaunched.
  if (_registered) {
    return;
  }

  DCHECK(brave::UpdateEnabled());
  DCHECK(_su_updater);

  [self updateStatus:kAutoupdateRegistering version:nil error:nil];

  _registered = true;

  [_su_updater setDelegate:self];

  // Background update check interval.
  constexpr NSTimeInterval kBraveUpdateCheckIntervalInSec = 3 * 60 * 60;
  [_su_updater setUpdateCheckInterval:kBraveUpdateCheckIntervalInSec];

  [_su_updater setAutomaticallyDownloadsUpdates:YES];

  // We only want to perform automatic update checks if we have write
  // access to the installation directory. Such access can be checked
  // with SUSystemUpdateInfo:systemAllowsAutomaticUpdatesForHost.
  // The following makes _su_updater call this method for us because
  // we setAutomaticallyDownloadUpdates:YES above.
  if ([_su_updater automaticallyDownloadsUpdates]) {
    [_su_updater setAutomaticallyChecksForUpdates:YES];
  }
  [self updateStatus:kAutoupdateRegistered version:nil error:nil];
}

- (NSString*)appInfoPlistPath {
  // NSBundle ought to have a way to access this path directly, but it
  // doesn't.
  return [[_appPath stringByAppendingPathComponent:@"Contents"]
      stringByAppendingPathComponent:@"Info.plist"];
}

// Returns the version of the currently-installed application on disk.
// If not, returns running version.
- (NSString*)currentlyInstalledVersion {
  // We don't know currently installed version from property list because
  // sparkle updates it when relaunching.
  // So, caching version when new candidate is found and use it.
  if (_updateWillBeInstalledOnQuit) {
    return _new_version;
  }

  NSString* appInfoPlistPath = [self appInfoPlistPath];
  NSDictionary* infoPlist =
      [NSDictionary dictionaryWithContentsOfFile:appInfoPlistPath];
  return base::apple::ObjCCast<NSString>(
      [infoPlist objectForKey:@"CFBundleShortVersionString"]);
}

- (void)checkForUpdates {
  DCHECK(_registered);

  if ([self asyncOperationPending]) {
    // Update check already in process; return without doing anything.
    return;
  }

  [self updateStatus:kAutoupdateChecking version:nil error:nil];

  [_su_updater checkForUpdatesInBackgroundWithoutUi];
}

- (BOOL)relaunch {
  if (_updateWillBeInstalledOnQuit && _su_updater.driver) {
    [_su_updater.driver installWithToolAndRelaunch:YES
                           displayingUserInterface:NO];
    return true;
  }
  return false;
}

- (void)checkForUpdatesInBackground {
  DCHECK(_registered);
  [_su_updater checkForUpdatesInBackgroundWithoutUi];
}

- (void)updateStatus:(AutoupdateStatus)status
             version:(NSString*)version
               error:(NSString*)error {
  NSMutableDictionary* dictionary =
      [NSMutableDictionary dictionaryWithObject:@(status)
                                         forKey:kAutoupdateStatusStatus];
  if (version.length) {
    dictionary[kAutoupdateStatusVersion] = version;
  }
  if (error.length) {
    dictionary[kAutoupdateStatusErrorMessages] = error;
  }

  NSNotification* notification =
      [NSNotification notificationWithName:kAutoupdateStatusNotification
                                    object:self
                                  userInfo:dictionary];
  _recentNotification = notification;

  [NSNotificationCenter.defaultCenter postNotification:notification];
}

- (BOOL)isOnReadOnlyFilesystem {
  const char* appPathC = _appPath.fileSystemRepresentation;
  struct statfs statfsBuf;

  if (statfs(appPathC, &statfsBuf) != 0) {
    PLOG(ERROR) << "statfs";
    // Be optimistic about the filesystem's writability.
    return NO;
  }

  return (statfsBuf.f_flags & MNT_RDONLY) != 0;
}

- (void)loadParameters {
  NSBundle* appBundle = base::apple::OuterBundle();
  NSString* appPath = appBundle.bundlePath;
  if (!appPath) {
    // If parameters required for sparkle are missing, don't use it.
    return;
  }

  _appPath = [appPath copy];
}

- (AutoupdateStatus)recentStatus {
  NSDictionary* dictionary = _recentNotification.userInfo;
  NSNumber* status = base::apple::ObjCCastStrict<NSNumber>(
      [dictionary objectForKey:kAutoupdateStatusStatus]);
  return static_cast<AutoupdateStatus>(status.intValue);
}

- (NSNotification*)recentNotification {
  return _recentNotification;
}

- (BOOL)asyncOperationPending {
  AutoupdateStatus status = [self recentStatus];
  return status == kAutoupdateRegistering ||
         status == kAutoupdateChecking ||
         status == kAutoupdateInstalling;
}

- (void)determineUpdateStatusAsync {
  DCHECK(NSThread.isMainThread);

  base::ThreadPool::PostTask(FROM_HERE,
                             {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
                              base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
                             base::BindOnce(^{
                               [self determineUpdateStatus];
                             }));
}

// Runs on a thread managed by WorkerPool.
- (void)determineUpdateStatus {
  DCHECK(!NSThread.isMainThread);

  NSString* version = [self currentlyInstalledVersion];

  [self performSelectorOnMainThread:@selector(determineUpdateStatusForVersion:)
                         withObject:version
                      waitUntilDone:NO];
}

- (void)determineUpdateStatusForVersion:(NSString*)version {
  DCHECK(NSThread.isMainThread);

  AutoupdateStatus status;
  if (_updateWillBeInstalledOnQuit) {
    // If this object was notified that an update will be installed, then don't
    // even bother to compare versions.
    status = kAutoupdateInstalled;
  } else {
    NSString* currentVersion = base::SysUTF8ToNSString(chrome::kChromeVersion);
    if (!version) {
      // If the version on disk could not be determined, assume that
      // whatever's running is current.
      version = currentVersion;
      status = kAutoupdateCurrent;
    } else if ([version isEqualToString:currentVersion]) {
      status = kAutoupdateCurrent;
    } else {
      // If the version on disk doesn't match what's currently running, an
      // update must have been applied in the background, without this app's
      // direct participation.  Leave _updateWillBeInstalledOnQuit alone
      // because there's no direct knowledge of what actually happened.
      status = kAutoupdateInstalled;
    }
  }

  [self updateStatus:status version:version error:nil];
}

#pragma mark - SUUpdaterDelegate

- (void)updater:(id)updater didFinishLoadingAppcast:(id)appcast {
  VLOG(0) << "brave update: did finish loading appcast";

  [self updateStatus:kAutoupdateChecking version:nil error:nil];
}

- (void)updater:(id)updater didFindValidUpdate:(id)item {
  VLOG(0) << "brave update: did find valid update with " +
             GetDescriptionFromAppcastItem(item);

  // Caching update candidate version.
  // See the comments of |currentlyInstalledVersion|.
  _new_version = GetVersionFromAppcastItem(item);

  [self updateStatus:kAutoupdateAvailable
             version:GetVersionFromAppcastItem(item)
               error:nil];
}

- (void)updaterDidNotFindUpdate:(id)updater {
  VLOG(0) << "brave update: did not find update";

  [self determineUpdateStatusAsync];
}

- (void)updater:(id)updater
    willDownloadUpdate:(id)item
           withRequest:(NSMutableURLRequest *)request {
  VLOG(0) << "brave update: willDownloadUpdate with " +
             GetDescriptionFromAppcastItem(item);
  [self updateStatus:kAutoupdateInstalling
             version:nil
               error:nil];
}

- (void)updater:(id)updater
    failedToDownloadUpdate:(id)item
                     error:(NSError *)error {
  VLOG(0) << "brave update: failed to download update with " +
                 GetDescriptionFromAppcastItem(item) + " with error - " +
                 base::SysNSStringToUTF8([error description]);
  [self updateStatus:kAutoupdateInstallFailed
             version:nil
               error:[error localizedDescription]];
}

- (void)userDidCancelDownload:(id)updater {
  VLOG(0) << "brave update: user did cancel download";
  [self updateStatus:kAutoupdateInstallFailed
               version:nil
                 error:nil];
}

- (void)updater:(id)updater willInstallUpdate:(id)item {
  VLOG(0) << "brave update: will install update with " +
             GetDescriptionFromAppcastItem(item);
  [self updateStatus:kAutoupdateInstalling
             version:nil
               error:nil];
}

- (void)updater:(id)updater
    willInstallUpdateOnQuit:(id)item
    immediateInstallationInvocation:(NSInvocation *)invocation {
  VLOG(0) << "brave update: will install update on quit with " +
             GetDescriptionFromAppcastItem(item);

  _updateWillBeInstalledOnQuit = YES;

  [self determineUpdateStatusAsync];
}

- (void)updater:(id)updater didAbortWithError:(NSError *)error {
  VLOG(0) << "brave update: did abort with error: " +
             base::SysNSStringToUTF8([error localizedDescription]);
  /* Error code. See SUErrors.h
    // Appcast phase errors.
    SUAppcastParseError = 1000,
    SUNoUpdateError = 1001,
    SUAppcastError = 1002,
    SURunningFromDiskImageError = 1003,

    // Download phase errors.
    SUTemporaryDirectoryError = 2000,
    SUDownloadError = 2001,

    // Extraction phase errors.
    SUUnarchivingError = 3000,
    SUSignatureError = 3001,

    // Installation phase errors.
    SUFileCopyFailure = 4000,
    SUAuthenticationFailure = 4001,
    SUMissingUpdateError = 4002,
    SUMissingInstallerToolError = 4003,
    SURelaunchError = 4004,
    SUInstallationError = 4005,
    SUDowngradeError = 4006,
    SUInstallationCancelledError = 4007,

    // System phase errors
    SUSystemPowerOffError = 5000
  */
  const int error_code = [error code];
  // SUNoUpdateError. Just return.
  // Status is updated by updaterDidNotFindUpdate().
  if (error_code == 1001)
    return;

  AutoupdateStatus status;
  // Treats 1XXX errors as checking error and all others are as install failed.
  if (error_code < 2000)
    status = kAutoupdateCheckFailed;
  else
    status = kAutoupdateInstallFailed;

  [self updateStatus:status
             version:nil
               error:[error localizedDescription]];
}

- (NSString*)feedURLStringForUpdater:(id)__unused updater {
  auto* command = base::CommandLine::ForCurrentProcess();
  if (command->HasSwitch(switches::kUpdateFeedURL)) {
    return base::SysUTF8ToNSString(
        command->GetSwitchValueASCII(switches::kUpdateFeedURL));
  }

  return [NSString stringWithFormat:@"https://updates.bravesoftware.com/"
                                    @"sparkle/Brave-Browser/%s/appcast.xml",
                                    GetUpdateChannel().c_str()];
}
@end

namespace sparkle_glue {

bool SparkleEnabled() {
  return [SparkleGlue sharedSparkleGlue] != nil;
}

std::u16string CurrentlyInstalledVersion() {
  SparkleGlue* sparkleGlue = [SparkleGlue sharedSparkleGlue];
  NSString* version = [sparkleGlue currentlyInstalledVersion];
  return base::SysNSStringToUTF16(version);
}

}  // namespace sparkle_glue
