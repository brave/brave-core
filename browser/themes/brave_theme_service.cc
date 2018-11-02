/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"

// static
void BraveThemeService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kBraveThemeType, BRAVE_THEME_TYPE_DEFAULT);
}

// static
BraveThemeType BraveThemeService::GetUserPreferredBraveThemeType(
                                                      Profile* profile) {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kUiMode)) {
    std::string requested_theme_value =
        command_line.GetSwitchValueASCII(switches::kUiMode);
    std::string requested_theme_value_lower =
        base::ToLowerASCII(requested_theme_value);
    if (requested_theme_value_lower == "light")
      return BraveThemeType::BRAVE_THEME_TYPE_LIGHT;
    if (requested_theme_value_lower == "dark")
      return BraveThemeType::BRAVE_THEME_TYPE_DARK;
  }
  // get value from preferences
  return static_cast<BraveThemeType>(
      profile->GetPrefs()->GetInteger(kBraveThemeType));
}

// static
std::string BraveThemeService::GetStringFromBraveThemeType(
    BraveThemeType type) {
  switch (type) {
    case BraveThemeType::BRAVE_THEME_TYPE_DEFAULT:
      return "Default";
    case BraveThemeType::BRAVE_THEME_TYPE_LIGHT:
      return "Light";
    case BraveThemeType::BRAVE_THEME_TYPE_DARK:
      return "Dark";
    default:
      NOTREACHED();
  }
}

// static
BraveThemeType BraveThemeService::GetActiveBraveThemeType(
                                                    Profile* profile) {
  const BraveThemeType preferred_theme =
                                        GetUserPreferredBraveThemeType(profile);
  switch (preferred_theme) {
    case BraveThemeType::BRAVE_THEME_TYPE_DEFAULT:
      switch (chrome::GetChannel()) {
        case version_info::Channel::STABLE:
        case version_info::Channel::BETA:
          return BraveThemeType::BRAVE_THEME_TYPE_LIGHT;
        case version_info::Channel::DEV:
        case version_info::Channel::CANARY:
        case version_info::Channel::UNKNOWN:
        default:
          return BraveThemeType::BRAVE_THEME_TYPE_DARK;
      }
    default:
      return preferred_theme;
  }
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
  // Brave Tor profiles are always 'incognito' (for now)
  if (!incognito && profile()->IsTorProfile())
    incognito = true;
  const BraveThemeType theme = GetActiveBraveThemeType(profile());
  const base::Optional<SkColor> braveColor =
      MaybeGetDefaultColorForBraveUi(id, incognito, theme);
  if (braveColor)
      return braveColor.value();
  // make sure we fallback to chrome's dark theme (incognito) for our dark theme
  if (theme == BraveThemeType::BRAVE_THEME_TYPE_DARK)
    incognito = true;
  return ThemeService::GetDefaultColor(id, incognito);
}

void BraveThemeService::OnPreferenceChanged(const std::string& pref_name) {
  DCHECK(pref_name == kBraveThemeType);
  NotifyThemeChanged();

  if (!brave_theme_event_router_)
    brave_theme_event_router_ = extensions::BraveThemeEventRouter::Create();

  brave_theme_event_router_->OnBraveThemeTypeChanged(profile());
}


void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}
