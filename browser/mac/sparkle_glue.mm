/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/sparkle_glue.h"

#include <string>
#include <sys/mount.h>
#include <sys/stat.h>

#include "base/command_line.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#import "base/mac/scoped_nsautorelease_pool.h"
#import "base/mac/scoped_nsobject.h"
#include "base/memory/ref_counted.h"
#include "base/strings/sys_string_conversions.h"
#include "base/system/sys_info.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#import "brave/browser/mac/su_updater.h"
#include "brave/browser/update_util.h"
#include "brave/common/brave_channel_info.h"
#include "brave/common/brave_switches.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_constants.h"

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
  return [description UTF8String];
}

// Adaptor for scheduling an Objective-C method call in TaskScheduler.
class PerformBridge : public base::RefCountedThreadSafe<PerformBridge> {
 public:

  // Call |sel| on |target| with |arg| in a WorkerPool thread.
  // |target| and |arg| are retained, |arg| may be |nil|.
  static void PostPerform(id target, SEL sel, id arg) {
    DCHECK(target);
    DCHECK(sel);

    scoped_refptr<PerformBridge> op = new PerformBridge(target, sel, arg);
    base::ThreadPool::PostTask(
        FROM_HERE,
        {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&PerformBridge::Run, std::move(op)));
  }

  // Convenience for the no-argument case.
  static void PostPerform(id target, SEL sel) {
    PostPerform(target, sel, nil);
  }

 private:
  // Allow RefCountedThreadSafe<> to delete.
  friend class base::RefCountedThreadSafe<PerformBridge>;

  PerformBridge(id target, SEL sel, id arg)
      : target_([target retain]),
        sel_(sel),
        arg_([arg retain]) {
  }

  ~PerformBridge() {}

  // Happens on a WorkerPool thread.
  void Run() {
    base::mac::ScopedNSAutoreleasePool pool;
    [target_ performSelector:sel_ withObject:arg_];
  }

  base::scoped_nsobject<id> target_;
  SEL sel_;
  base::scoped_nsobject<id> arg_;
};

}  // namespace

@implementation SparkleGlue
{
  SUUpdater* su_updater_;

  BOOL registered_;
  BOOL updateSuccessfullyInstalled_;

  NSString* appPath_;
  NSString* new_version_;

  // The most recent kAutoupdateStatusNotification notification posted.
  base::scoped_nsobject<NSNotification> recentNotification_;
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
  [new_version_ release];

  [super dealloc];
}

- (BOOL)loadSparkleFramework {
  if (!appPath_)
    return NO;

  if ([self isOnReadOnlyFilesystem])
    return NO;

  DCHECK(!su_updater_);

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
  // This can be called by BraveBrowserMainPartsMac::PreMainMessageLoopStart()
  // again when browser is relaunched.
  if (registered_)
    return;

  DCHECK(brave::UpdateEnabled());
  DCHECK(su_updater_);

  [self updateStatus:kAutoupdateRegistering version:nil error:nil];

  registered_ = true;

  [su_updater_ setDelegate:self];

  // Background update check interval.
  constexpr NSTimeInterval kBraveUpdateCheckIntervalInSec = 3 * 60 * 60;
  [su_updater_ setUpdateCheckInterval:kBraveUpdateCheckIntervalInSec];
  [su_updater_ setAutomaticallyChecksForUpdates:YES];
  [su_updater_ setAutomaticallyDownloadsUpdates:YES];

  [self updateStatus:kAutoupdateRegistered version:nil error:nil];
}

- (NSString*)appInfoPlistPath {
  // NSBundle ought to have a way to access this path directly, but it
  // doesn't.
  return [[appPath_ stringByAppendingPathComponent:@"Contents"]
             stringByAppendingPathComponent:@"Info.plist"];
}

// Returns the version of the currently-installed application on disk.
// If not, returns running version.
- (NSString*)currentlyInstalledVersion {
  // We don't know currently installed version from property list because
  // sparkle updates it when relaunching.
  // So, caching version when new candidate is found and use it.
  if (updateSuccessfullyInstalled_)
    return new_version_;

  NSString* appInfoPlistPath = [self appInfoPlistPath];
  NSDictionary* infoPlist =
      [NSDictionary dictionaryWithContentsOfFile:appInfoPlistPath];
  return base::mac::ObjCCast<NSString>(
      [infoPlist objectForKey:@"CFBundleShortVersionString"]);
}

- (void)checkForUpdates {
  DCHECK(registered_);

  if ([self asyncOperationPending]) {
    // Update check already in process; return without doing anything.
    return;
  }

  [self updateStatus:kAutoupdateChecking version:nil error:nil];

  [su_updater_ checkForUpdatesInBackground];
}

- (void)relaunch {
  [su_updater_.driver installWithToolAndRelaunch:YES
                         displayingUserInterface:NO];
}

- (void)checkForUpdatesInBackground {
  DCHECK(registered_);
  [su_updater_ checkForUpdatesInBackground];
}

- (void)updateStatus:(AutoupdateStatus)status
             version:(NSString*)version
               error:(NSString*)error {
  NSNumber* statusNumber = [NSNumber numberWithInt:status];
  NSMutableDictionary* dictionary =
      [NSMutableDictionary dictionaryWithObject:statusNumber
                                         forKey:kAutoupdateStatusStatus];
  if ([version length]) {
    [dictionary setObject:version forKey:kAutoupdateStatusVersion];
  }
  if ([error length]) {
    [dictionary setObject:error forKey:kAutoupdateStatusErrorMessages];
  }

  NSNotification* notification =
      [NSNotification notificationWithName:kAutoupdateStatusNotification
                                    object:self
                                  userInfo:dictionary];
  recentNotification_.reset([notification retain]);

  [[NSNotificationCenter defaultCenter] postNotification:notification];
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
  NSString* appPath = [appBundle bundlePath];
  if (!appPath) {
    // If parameters required for sparkle are missing, don't use it.
    return;
  }

  appPath_ = [appPath retain];
}

- (AutoupdateStatus)recentStatus {
  NSDictionary* dictionary = [recentNotification_ userInfo];
  NSNumber* status = base::mac::ObjCCastStrict<NSNumber>(
      [dictionary objectForKey:kAutoupdateStatusStatus]);
  return static_cast<AutoupdateStatus>([status intValue]);
}

- (NSNotification*)recentNotification {
  return [[recentNotification_ retain] autorelease];
}

- (BOOL)asyncOperationPending {
  AutoupdateStatus status = [self recentStatus];
  return status == kAutoupdateRegistering ||
         status == kAutoupdateChecking ||
         status == kAutoupdateInstalling;
}

- (void)determineUpdateStatusAsync {
  DCHECK([NSThread isMainThread]);

  PerformBridge::PostPerform(self, @selector(determineUpdateStatus));
}

// Runs on a thread managed by WorkerPool.
- (void)determineUpdateStatus {
  DCHECK(![NSThread isMainThread]);

  NSString* version = [self currentlyInstalledVersion];

  [self performSelectorOnMainThread:@selector(determineUpdateStatusForVersion:)
                         withObject:version
                      waitUntilDone:NO];
}

- (void)determineUpdateStatusForVersion:(NSString*)version {
  DCHECK([NSThread isMainThread]);

  AutoupdateStatus status;
  if (updateSuccessfullyInstalled_) {
    // If an update was successfully installed and this object saw it happen,
    // then don't even bother comparing versions.
    status = kAutoupdateInstalled;
  } else {
    NSString* currentVersion =
        [NSString stringWithUTF8String:chrome::kChromeVersion];
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
      // direct participation.  Leave updateSuccessfullyInstalled_ alone
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
  new_version_ = [GetVersionFromAppcastItem(item) retain];

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
             GetDescriptionFromAppcastItem(item) +
             " with error - " + [[error description] UTF8String];
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

  updateSuccessfullyInstalled_ = YES;

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
