/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include "brave/browser/themes/theme_properties.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

// static
void BraveThemeService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kBraveThemeType, BRAVE_THEME_TYPE_DEFAULT);
}

// static
int BraveThemeService::GetBraveThemeType(Profile* profile) {
  return profile->GetPrefs()->GetInteger(kBraveThemeType);
}

// static
void BraveThemeService::SetBraveThemeType(Profile* profile,
	                                      BraveThemeType type) {
  profile->GetPrefs()->SetInteger(kBraveThemeType, type);

  static_cast<BraveThemeService*>(ThemeServiceFactory::GetForProfile(profile))
      ->NotifyThemeChanged();
}

SkColor BraveThemeService::GetDefaultColor(int id, bool incognito) const {
  const base::Optional<SkColor> braveColor =
      MaybeGetDefaultColorForBraveUi(id, incognito, profile());
  if (braveColor)
      return braveColor.value();

  return ThemeService::GetDefaultColor(id, incognito);
}
