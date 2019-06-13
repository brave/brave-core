/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/global_error/brave_global_error_service.h"

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "content/public/browser/notification_service.h"

BraveGlobalErrorService::BraveGlobalErrorService(Profile* profile)
  : GlobalErrorService(profile),
    profile_(profile) {
}

BraveGlobalErrorService::~BraveGlobalErrorService() {
}

// GlobalErrorService::NotifyErrorsChanged send notifications to both original
// and its associated OTR profile. Send the notification to associated Tor
// profile here to update Tor windows as well.
void BraveGlobalErrorService::NotifyErrorsChanged(GlobalError* error) {
  GlobalErrorService::NotifyErrorsChanged(error);
  if (!profile_ || !profile_->HasTorProfile())
    return;

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED,
      content::Source<Profile>(profile_->GetTorProfile()),
      content::Details<GlobalError>(error));
}
