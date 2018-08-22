/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_util.h"

#include "brave/common/pref_names.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/notification_service.h"

int GetBraveThemeType(Profile* profile) {
  return profile->GetPrefs()->GetInteger(kBraveThemeType);
}

void SetBraveThemeType(Profile* profile, BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);

  content::NotificationService* service =
      content::NotificationService::current();
  service->Notify(chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
                  content::Source<ThemeService>(
                      ThemeServiceFactory::GetForProfile(profile)),
                  content::NotificationService::NoDetails());
}

void RegisterProfilePrefsForBraveThemeType(
	user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kBraveThemeType, BRAVE_THEME_TYPE_DEFAULT);
}
