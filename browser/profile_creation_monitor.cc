/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profile_creation_monitor.h"

#include "brave/browser/alternate_private_search_engine_controller.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_service.h"

ProfileCreationMonitor::ProfileCreationMonitor() {
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CREATED,
                 content::NotificationService::AllSources());
}

ProfileCreationMonitor::~ProfileCreationMonitor() {}

void ProfileCreationMonitor::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      Profile* profile = content::Source<Profile>(source).ptr();
      if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE)
        AlternatePrivateSearchEngineController::Create(profile);
      break;
    }
    default:
      NOTREACHED();
  }
}
