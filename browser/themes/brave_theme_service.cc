/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_service.h"

#include <utility>

#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "brave/browser/extensions/brave_theme_event_router.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/themes/brave_theme_utils.h"
#include "brave/common/brave_switches.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_dark_aura.h"

namespace {
BraveThemeType GetThemeTypeBasedOnChannel() {
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
}
}  // namespace

// static
std::string BraveThemeService::GetStringFromBraveThemeType(
    BraveThemeType type) {
  switch (type) {
    case BraveThemeType::BRAVE_THEME_TYPE_LIGHT:
      return "Light";
    case BraveThemeType::BRAVE_THEME_TYPE_DARK:
      return "Dark";
    default:
      NOTREACHED();
      return "Default";
  }
}

// static
base::Value BraveThemeService::GetBraveThemeList() {
  base::Value list(base::Value::Type::LIST);

  if (SystemThemeModeEnabled()) {
    base::Value system_type(base::Value::Type::DICTIONARY);
    system_type.SetKey(
        "value",
        base::Value(BraveThemeType::BRAVE_THEME_TYPE_DEFAULT));
    system_type.SetKey(
        "name",
        base::Value(l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_SYSTEM)));
    list.GetList().push_back(std::move(system_type));
  }

  base::Value dark_type(base::Value::Type::DICTIONARY);
  dark_type.SetKey("value", base::Value(BraveThemeType::BRAVE_THEME_TYPE_DARK));
  dark_type.SetKey(
      "name",
      base::Value(l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_DARK)));
  list.GetList().push_back(std::move(dark_type));

  base::Value light_type(base::Value::Type::DICTIONARY);
  light_type.SetKey("value",
                    base::Value(BraveThemeType::BRAVE_THEME_TYPE_LIGHT));
  light_type.SetKey(
      "name",
      base::Value(l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_LIGHT)));
  list.GetList().push_back(std::move(light_type));

  return list;
}

// static
void BraveThemeService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kBraveThemeType, BRAVE_THEME_TYPE_DEFAULT);

  // When this is set to true, prefs is changed from default type to
  // effective type. In dtor, pref is reverted to default type if this is
  // still true. With this, we can preserve the context that user didn't touch
  // theme type yet. If it is changed to false, it means user changes system
  // theme explicitly.
  // To handle crash case, prefs is used instead of boolean flags. Recovering
  // is done in BraveThemeService::Init().
  registry->RegisterBooleanPref(kUseOverriddenBraveThemeType, false);
}

// static
BraveThemeType BraveThemeService::GetActiveBraveThemeType(
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

  BraveThemeType type = static_cast<BraveThemeType>(
      profile->GetPrefs()->GetInteger(kBraveThemeType));
  if (type == BraveThemeType::BRAVE_THEME_TYPE_DEFAULT) {
    DCHECK(SystemThemeModeEnabled());
    return ui::NativeTheme::GetInstanceForNativeUi()->
         SystemDarkModeEnabled() ? BraveThemeType::BRAVE_THEME_TYPE_DARK
                                 : BraveThemeType::BRAVE_THEME_TYPE_LIGHT;
  }
  return type;
}

BraveThemeService::BraveThemeService() {}

BraveThemeService::~BraveThemeService() {
  // In test, kBraveThemeType isn't registered.
  if (!profile()->GetPrefs()->FindPreference(kBraveThemeType))
    return;

  if (profile()->GetPrefs()->GetBoolean(kUseOverriddenBraveThemeType)) {
    brave_theme_type_pref_.Destroy();
    profile()->GetPrefs()->SetInteger(kBraveThemeType,
                                      BraveThemeType::BRAVE_THEME_TYPE_DEFAULT);
  }
}

void BraveThemeService::Init(Profile* profile) {
  // In test, kBraveThemeType isn't registered.
  if (profile->GetPrefs()->FindPreference(kBraveThemeType)) {
    RecoverPrefStates(profile);
    OverrideDefaultThemeIfNeeded(profile);
    if (SystemThemeModeEnabled()) {
      // Start with proper system theme to make brave theme and
      // base ui components theme use same theme.
      SetSystemTheme(static_cast<BraveThemeType>(
          profile->GetPrefs()->GetInteger(kBraveThemeType)));
    }

    brave_theme_type_pref_.Init(
      kBraveThemeType,
      profile->GetPrefs(),
      base::Bind(&BraveThemeService::OnPreferenceChanged,
                 base::Unretained(this)));

    brave_theme_event_router_.reset(
        new extensions::BraveThemeEventRouter(profile));
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

  // Changing theme type means default theme is not overridden anymore.
  profile()->GetPrefs()->SetBoolean(kUseOverriddenBraveThemeType, false);

  bool notify_theme_observer_here = true;
#if defined(OS_MACOSX)
  if (SystemThemeModeEnabled()) {
    // When system theme is changed, system theme changing observer notifies
    // proper native theme observers.
    // So, we don't need to notify again. See NotifyProperThemeObserver()
    // in chromium_src/ui/native_theme/native_theme_mac.mm.
    notify_theme_observer_here = false;
    SetSystemTheme(static_cast<BraveThemeType>(
        profile()->GetPrefs()->GetInteger(kBraveThemeType)));
  }
#endif
  if (notify_theme_observer_here) {
    // Notify dark (cross-platform) and light (platform-specific) variants
    // When theme is changed from light to dark, we notify to light theme
    // observer because NativeThemeObserver observes light native theme.
    GetActiveBraveThemeType(profile()) == BraveThemeType::BRAVE_THEME_TYPE_LIGHT
        ? ui::NativeThemeDarkAura::instance()->NotifyObservers()
        : ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers();
  }
}

void BraveThemeService::RecoverPrefStates(Profile* profile) {
  // kUseOverriddenBraveThemeType is true means pref states are not cleaned
  // up properly at the last running(ex, crash). Recover them here.
  if (profile->GetPrefs()->GetBoolean(kUseOverriddenBraveThemeType)) {
    profile->GetPrefs()->SetInteger(kBraveThemeType,
                                    BraveThemeType::BRAVE_THEME_TYPE_DEFAULT);
  }
}

void BraveThemeService::OverrideDefaultThemeIfNeeded(Profile* profile) {
  if (!SystemThemeModeEnabled() &&
      profile->GetPrefs()->GetInteger(kBraveThemeType) ==
          BraveThemeType::BRAVE_THEME_TYPE_DEFAULT) {
    profile->GetPrefs()->SetBoolean(kUseOverriddenBraveThemeType,
                                    true);
    profile->GetPrefs()->SetInteger(kBraveThemeType,
                                    GetThemeTypeBasedOnChannel());
  }
}

void BraveThemeService::SetBraveThemeEventRouterForTesting(
    extensions::BraveThemeEventRouter* mock_router) {
  brave_theme_event_router_.reset(mock_router);
}

// static
bool BraveThemeService::use_system_theme_mode_in_test_ = false;
bool BraveThemeService::is_test_ = false;

// static
bool BraveThemeService::SystemThemeModeEnabled() {
  if (is_test_)
    return use_system_theme_mode_in_test_;

  if (!base::FeatureList::IsEnabled(features::kDarkMode))
    return false;

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kForceDarkMode))
    return true;

  return SystemThemeSupportDarkMode();
}
