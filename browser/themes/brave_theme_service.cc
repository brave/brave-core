/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include <string>

#include "brave/common/brave_switches.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/pref_names.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

// static
void BraveThemeService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kBraveThemeType, BRAVE_THEME_TYPE_DEFAULT);
}

// static
BraveThemeService::BraveThemeType BraveThemeService::GetBraveThemeType(Profile* profile) {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kUiMode)) {
    std::string requested_theme_value = command_line.GetSwitchValueASCII(switches::kUiMode);
    std::string requested_theme_value_lower = base::ToLowerASCII(requested_theme_value);
    if (requested_theme_value_lower == "light")
      return BraveThemeService::BraveThemeType::BRAVE_THEME_TYPE_LIGHT;
    if (requested_theme_value_lower == "light")
      return BraveThemeService::BraveThemeType::BRAVE_THEME_TYPE_DARK;
  }
  // get value from preferences
  return static_cast<BraveThemeService::BraveThemeType>(
      profile->GetPrefs()->GetInteger(kBraveThemeType));
}

BraveThemeService::BraveThemeService() {}

BraveThemeService::~BraveThemeService() {}

void BraveThemeService::Init(Profile* profile) {
  // In unittest, kBraveThemeType isn't registered.
  if (profile->GetPrefs()->FindPreference(kBraveThemeType)) {
    brave_theme_type_pref_.Init(
      kBraveThemeType,
      profile->GetPrefs(),
      base::Bind(&BraveThemeService::OnPreferenceChanged,
                 base::Unretained(this)));
  }
  ThemeService::Init(profile);
}

SkColor BraveThemeService::GetDefaultColor(int id, bool incognito) const {
  const base::Optional<SkColor> braveColor =
      MaybeGetDefaultColorForBraveUi(id, incognito, profile());
  if (braveColor)
      return braveColor.value();

  return ThemeService::GetDefaultColor(id, incognito);
}

void BraveThemeService::OnPreferenceChanged(const std::string& pref_name) {
  DCHECK(pref_name == kBraveThemeType);
  NotifyThemeChanged();
}
