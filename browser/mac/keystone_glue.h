/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_KEYSTONE_GLUE_H_
#define BRAVE_BROWSER_MAC_KEYSTONE_GLUE_H_

#include <string>

#if defined(__OBJC__)

#import <Foundation/Foundation.h>
#include <stdint.h>

#include "base/mac/scoped_authorizationref.h"

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
  kAutoupdatePromoting,       // no version (asynchronous operation in progress)
  kAutoupdatePromoted,        // no version
  kAutoupdateRegisterFailed,  // no version
  kAutoupdateCheckFailed,     // no version
  kAutoupdateInstallFailed,   // no version
  kAutoupdatePromoteFailed,   // no version
  kAutoupdateNeedsPromotion,  // no version
};

// kAutoupdateStatusNotification is the name of the notification posted when
// -checkForUpdate and -installUpdate complete.  This notification will be
// sent with with its sender object set to the KeystoneGlue instance sending
// the notification.  Its userInfo dictionary will contain an AutoupdateStatus
// value as an intValue at key kAutoupdateStatusStatus.  If a version is
// available (see AutoupdateStatus), it will be present at key
// kAutoupdateStatusVersion.  If any error messages were supplied by Keystone,
// they will be present at key kAutoupdateStatusErrorMessages.
extern NSString* const kAutoupdateStatusNotification;
extern NSString* const kAutoupdateStatusStatus;
extern NSString* const kAutoupdateStatusVersion;
extern NSString* const kAutoupdateStatusErrorMessages;

// KeystoneGlue is an adapter around the KSRegistration class, allowing it to
// be used without linking directly against its containing KeystoneRegistration
// framework.  This is used in an environment where most builds (such as
// developer builds) don't want or need Keystone support and might not even
// have the framework available.  Enabling Keystone support in an application
// that uses KeystoneGlue is as simple as dropping
// KeystoneRegistration.framework in the application's Frameworks directory
// and providing the relevant information in its Info.plist.  KeystoneGlue
// requires that the KSUpdateURL key be set in the application's Info.plist,
// and that it contain a string identifying the update URL to be used by
// Keystone.

@class KSRegistration;

@interface KeystoneGlue : NSObject

// Return the default Keystone Glue object.
+ (KeystoneGlue*)defaultKeystoneGlue;  // NOLINT

// Load KeystoneRegistration.framework if present, call into it to register
// with Keystone, and set up periodic activity pings.
- (void)registerWithKeystone;
- (BOOL)isRegisteredAndActive;

// -checkForUpdate launches a check for updates, and -installUpdate begins
// installing an available update.  For each, status will be communicated via
// a kAutoupdateStatusNotification notification, and will also be available
// through -recentNotification.
- (void)checkForUpdate;
- (void)installUpdate;

// Accessor for recentNotification_.  Returns an autoreleased NSNotification.
- (NSNotification*)recentNotification;  // NOLINT

// Accessor for recentNotification_.  Returns an autoreleased NSNotification.
- (NSNotification*)recentNotification;  // NOLINT

// Accessor for the kAutoupdateStatusStatus field of recentNotification_'s
// userInfo dictionary.
- (AutoupdateStatus)recentStatus;

// Returns YES if an asynchronous operation is pending: if an update check or
// installation attempt is currently in progress.
- (BOOL)asyncOperationPending;

// Returns YES if the application is running from a read-only filesystem,
// such as a disk image.
- (BOOL)isOnReadOnlyFilesystem;

// -needsPromotion is YES if the application needs its ticket promoted to
// a system ticket.  This will be YES when the application is on a user
// ticket and determines that the current user does not have sufficient
// permission to perform the update.
//
// -wantsPromotion is YES if the application wants its ticket promoted to
// a system ticket, even if it doesn't need it as determined by
// -needsPromotion.  -wantsPromotion will always be YES if -needsPromotion is,
// and it will additionally be YES when the application is on a user ticket
// and appears to be installed in a system-wide location such as
// /Applications.
//
// Use -needsPromotion to decide whether to show any update UI at all.  If
// it's YES, there's no sense in asking the user to "update now" because it
// will fail given the rights and permissions involved.  On the other hand,
// when -needsPromotion is YES, the application can encourage the user to
// promote the ticket so that updates will work properly.
//
// Use -wantsPromotion to decide whether to allow the user to promote.  The
// user shouldn't be nagged about promotion on the basis of -wantsPromotion,
// but if it's YES, the user should be allowed to promote the ticket.
- (BOOL)needsPromotion;
- (BOOL)wantsPromotion;

// -isAutoupdateEnabledForAllUsers indicates whether or not autoupdate is
// turned on for all users.
- (BOOL)isAutoupdateEnabledForAllUsers;

// Promotes the Keystone ticket into the system store.  System Keystone will
// be installed if necessary.  If synchronous is NO, the promotion may occur
// in the background.  synchronous should be YES for promotion during
// installation.
- (void)promoteTicketWithAuthorization:
            (base::mac::ScopedAuthorizationRef)authorization
                           synchronous:(BOOL)synchronous;

// Requests authorization and calls -promoteTicketWithAuthorization: in
// asynchronous mode.
- (void)promoteTicket;

// Set the registration active.
- (void)setRegistrationActive;

// Sets a new value for appPath.  Used during installation to point a ticket
// at the installed copy.
- (void)setAppPath:(NSString*)appPath;  // NOLINT

@end  // @interface KeystoneGlue

@interface KeystoneGlue (ExposedForTesting)

@property(readonly) NSString* productID;
@property(readonly) NSString* url;
@property(readonly) NSString* version;
@property(readonly) NSTimer* timer;

// Load any params we need for configuring Keystone.
- (void)loadParameters;

// Load the Keystone registration object.
// Return NO on failure.
- (BOOL)loadKeystoneRegistration;

- (void)setKeystoneRegistration:(KSRegistration*)registration;  // NOLINT

- (void)stopTimer;

// Called when a checkForUpdate: notification completes.
- (void)checkForUpdateComplete:(NSNotification*)notification;  // NOLINT

// Called when an installUpdate: notification completes.
- (void)installUpdateComplete:(NSNotification*)notification;  // NOLINT

@end  // @interface KeystoneGlue (ExposedForTesting)

#endif  // __OBJC__

// Functions that may be accessed from non-Objective-C C/C++ code.
namespace keystone_glue {

// Returns the brand code of the installation. Note that beta, dev, and canary
// channels, as well as some stable builds, may have an empty string as a brand
// code.
std::string BrandCode();

// True if Keystone is enabled.
bool KeystoneEnabled();

// The version of the application currently installed on disk.
std::u16string CurrentlyInstalledVersion();

}  // namespace keystone_glue

#endif  // BRAVE_BROWSER_MAC_KEYSTONE_GLUE_H_
