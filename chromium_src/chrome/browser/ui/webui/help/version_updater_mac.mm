/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/help/version_updater_mac.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/sparkle_buildflags.h"
#include "chrome/browser/mac/keystone_glue.h"
#include "chrome/browser/obsolete_system/obsolete_system.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "net/base/escape.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#import "brave/browser/mac/sparkle_glue.h"
#endif

// KeystoneObserver is a simple notification observer for Keystone status
// updates. It will be created and managed by VersionUpdaterMac.
@interface KeystoneObserver : NSObject {
 @private
  VersionUpdaterMac* versionUpdater_;  // Weak.
}

// Initialize an observer with an updater. The updater owns this object.
- (id)initWithUpdater:(VersionUpdaterMac*)updater;

// Notification callback, called with the status of keystone operations.
- (void)handleStatusNotification:(NSNotification*)notification;

@end  // @interface KeystoneObserver

@implementation KeystoneObserver

- (id)initWithUpdater:(VersionUpdaterMac*)updater {
  if ((self = [super init])) {
    versionUpdater_ = updater;
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    [center addObserver:self
               selector:@selector(handleStatusNotification:)
                   name:kAutoupdateStatusNotification
                 object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)handleStatusNotification:(NSNotification*)notification {
  versionUpdater_->UpdateStatus([notification userInfo]);
}

@end  // @implementation KeystoneObserver


VersionUpdater* VersionUpdater::Create(
    content::WebContents* web_contents) {
  return new VersionUpdaterMac;
}

VersionUpdaterMac::VersionUpdaterMac()
    : keystone_observer_([[KeystoneObserver alloc] initWithUpdater:this]) {
  show_promote_button_ = false;
}

VersionUpdaterMac::~VersionUpdaterMac() {
}

void VersionUpdaterMac::CheckForUpdate(StatusCallback status_callback,
                                       PromoteCallback promote_callback) {
  status_callback_ = std::move(status_callback);

#if BUILDFLAG(ENABLE_SPARKLE)
  if (SparkleGlue* sparkle_glue = [SparkleGlue sharedSparkleGlue]) {
    AutoupdateStatus recent_status = [sparkle_glue recentStatus];
    if ([sparkle_glue asyncOperationPending] ||
        recent_status == kAutoupdateInstalled ||
        recent_status == kAutoupdateRegisterFailed) {
      // If installation is finished and new version is waiting for relaunch,
      // set the status up correspondingly without launching a new update check.
      //
      // If an asynchronous update operation is currently pending, such as a
      // check for updates or an update installation attempt, set the status
      // up correspondingly without launching a new update check.
      //
      // If registration failed, no other operations make sense, so just go
      // straight to the error.
      UpdateStatus([[sparkle_glue recentNotification] userInfo]);
    } else {
      // Launch a new update check, even if one was already completed, because
      // a new update may be available or a new update may have been installed
      // in the background since the last time the Help page was displayed.
      [sparkle_glue checkForUpdates];

      // Immediately, kAutoupdateStatusNotification will be posted, with status
      // kAutoupdateChecking.
      //
      // Upon completion, kAutoupdateStatusNotification will be posted with a
      // status indicating the result of the check.
    }
  } else {
    status_callback_.Run(DISABLED, 0, false, false, std::string(), 0,
                         base::string16());
  }
#else
  status_callback_.Run(DISABLED, 0, false, false, std::string(), 0,
                         base::string16());
#endif
}

void VersionUpdaterMac::PromoteUpdater() const {
  NOTIMPLEMENTED();
}

void VersionUpdaterMac::UpdateStatus(NSDictionary* dictionary) {
  AutoupdateStatus sparkle_status = static_cast<AutoupdateStatus>(
      [base::mac::ObjCCastStrict<NSNumber>(
          [dictionary objectForKey:kAutoupdateStatusStatus]) intValue]);
  std::string error_messages = base::SysNSStringToUTF8(
      base::mac::ObjCCastStrict<NSString>(
          [dictionary objectForKey:kAutoupdateStatusErrorMessages]));

  base::string16 message;

  Status status;
  switch (sparkle_status) {
    case kAutoupdateRegistering:
    case kAutoupdateChecking:
      status = CHECKING;
      break;

    case kAutoupdateRegistered:
      // Go straight into an update check. Return immediately, this routine
      // will be re-entered shortly with kAutoupdateChecking.
#if BUILDFLAG(ENABLE_SPARKLE)
      [[SparkleGlue sharedSparkleGlue] checkForUpdates];
#endif
      return;

    case kAutoupdateCurrent:
      status = UPDATED;
      break;

    case kAutoupdateAvailable:
      // Do nothing with this status.
      // Sparkle will start installing automatically.
      // Will receive |kAutoupdateInstalling| immediately.
      return;

    case kAutoupdateInstalling:
      status = UPDATING;
      break;

    case kAutoupdateInstalled:
      status = NEARLY_UPDATED;
      break;

    case kAutoupdateRegisterFailed:
    case kAutoupdateCheckFailed:
    case kAutoupdateInstallFailed:
      status = FAILED;
      message = l10n_util::GetStringFUTF16Int(IDS_UPGRADE_ERROR,
                                              sparkle_status);
      break;

    default:
      NOTREACHED();
      return;
  }

  // If there are any detailed error messages being passed along by Keystone,
  // log them. If we have an error to display, include the detail messages
  // below the error in a <pre> block. Don't bother displaying detail messages
  // on a success/in-progress/indeterminate status.
  if (!error_messages.empty()) {
    VLOG(1) << "Update error messages: " << error_messages;

    if (status == FAILED) {
      if (!message.empty()) {
        message += base::UTF8ToUTF16("<br/><br/>");
      }

      message += l10n_util::GetStringUTF16(IDS_UPGRADE_ERROR_DETAILS);
      message += base::UTF8ToUTF16("<br/><pre>");
      message += base::UTF8ToUTF16(net::EscapeForHTML(error_messages));
      message += base::UTF8ToUTF16("</pre>");
    }
  }

  if (!status_callback_.is_null())
    status_callback_.Run(status, 0, false, false, std::string(), 0, message);
}


void VersionUpdaterMac::UpdateShowPromoteButton() {
  NOTIMPLEMENTED();
}
