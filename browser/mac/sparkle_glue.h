/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
#define BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_

#include "base/strings/string16.h"

#if defined(__OBJC__)

#import <Foundation/Foundation.h>

// Possible outcomes of various operations.  A version may accompany some of
// these, but beware: a version is never required.  For statuses that can be
// accompanied by a version, the comment indicates what version is referenced.
// A notification posted containing an asynchronous status will always be
// followed by a notification with a terminal status.
enum AutoupdateStatus {
  kAutoupdateNone = 0,        // no version (initial state only)
  kAutoupdateRegistering,     // no version (asynchronous operation in progress)
  kAutoupdateRegistered,      // no version
  kAutoupdateChecking,        // no version (asynchronous operation in progress)
  kAutoupdateCurrent,         // version of the running application
  kAutoupdateAvailable,       // version of the update that is available
  kAutoupdateInstalling,      // no version (asynchronous operation in progress)
  kAutoupdateInstalled,       // version of the update that was installed
  kAutoupdateRegisterFailed,  // no version
  kAutoupdateCheckFailed,     // no version
  kAutoupdateInstallFailed,   // no version
};

// kBraveAutoupdateStatusNotification is the name of the notification posted
// when -checkForUpdate and -installUpdate complete.  This notification will be
// sent with with its sender object set to the SparkleGlue instance sending
// the notification.  Its userInfo dictionary will contain an AutoupdateStatus
// value as an intValue at key kBraveAutoupdateStatusStatus.  If a version is
// available (see AutoupdateStatus), it will be present at key
// kBraveAutoupdateStatusVersion.  If any error messages were supplied by
// Sparkle, they will be present at key kBraveAutoupdateStatusErrorMessages.
extern NSString* const kBraveAutoupdateStatusNotification;
extern NSString* const kBraveAutoupdateStatusStatus;
extern NSString* const kBraveAutoupdateStatusVersion;
extern NSString* const kBraveAutoupdateStatusErrorMessages;

@interface SparkleGlue : NSObject

+ (instancetype)sharedSparkleGlue;

- (instancetype)init;

- (void)registerWithSparkle;

- (void)checkForUpdates;

- (AutoupdateStatus)recentStatus;
- (NSNotification*)recentNotification;

// Returns YES if an asynchronous operation is pending: if an update check or
// installation attempt is currently in progress.
- (BOOL)asyncOperationPending;

// Returns YES if the application is running from a read-only filesystem,
// such as a disk image.
- (BOOL)isOnReadOnlyFilesystem;

@end  // @interface SparkleGlue

#endif  // __OBJC__

namespace sparkle_glue {

bool SparkleEnabled();

base::string16 CurrentlyInstalledVersion();

}  // namespace sparkle_glue

#endif  // BRAVE_BROWSER_MAC_SPARKLE_GLUE_H_
