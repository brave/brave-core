/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/browser/themes/brave_dark_mode_utils_internal.h"
#include "brave/browser/ui/themes/switches.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/native_theme/native_theme.h"

namespace {

bool g_is_test_ = false;
bool g_system_dark_mode_enabled_in_test_ = false;

dark_mode::BraveDarkModeType GetDarkModeTypeBasedOnChannel() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
    default:
      return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }
}

dark_mode::BraveDarkModeType GetDarkModeSwitchValue(
    const base::CommandLine& command_line) {
  DCHECK(command_line.HasSwitch(switches::kDarkMode));

  std::string requested_dark_mode_value =
      command_line.GetSwitchValueASCII(switches::kDarkMode);
  std::string requested_dark_mode_value_lower =
      base::ToLowerASCII(requested_dark_mode_value);
  DCHECK(requested_dark_mode_value_lower == "dark" ||
         requested_dark_mode_value == "light");

  if (requested_dark_mode_value_lower == "light") {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }
  if (requested_dark_mode_value_lower == "dark") {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }

  return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
}

}  // namespace

namespace dark_mode {

void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBraveDarkMode,
      static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT));
}

bool SystemDarkModeEnabled() {
  if (g_is_test_) {
    return g_system_dark_mode_enabled_in_test_;
  }

#if BUILDFLAG(IS_LINUX)
  return HasCachedSystemDarkModeType();
#else
  return ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported();
#endif
}

void SetUseSystemDarkModeEnabledForTest(bool enabled) {
  g_is_test_ = true;
  g_system_dark_mode_enabled_in_test_ = enabled;
}

std::string GetStringFromBraveDarkModeType(BraveDarkModeType type) {
  DCHECK_NE(type, BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT)
      << "Didn't expect to encounter the default theme mode here - this was "
         "previously a NOTREACHED";
  switch (type) {
    case BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT:
      return "Light";
    case BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK:
      return "Dark";
    default:
      return "Default";
  }
}

void SetBraveDarkModeType(const std::string& type) {
  BraveDarkModeType parsed_type =
      BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT;

  if (type == "Light") {
    parsed_type = BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  } else if (type == "Dark") {
    parsed_type = BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }
  SetBraveDarkModeType(parsed_type);
}

void SetBraveDarkModeType(BraveDarkModeType type) {
  g_browser_process->local_state()->SetInteger(kBraveDarkMode,
                                               static_cast<int>(type));
}

BraveDarkModeType GetActiveBraveDarkModeType() {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDarkMode)) {
    return GetDarkModeSwitchValue(command_line);
  }

  if (!g_browser_process || !g_browser_process->local_state()) {
    // In unittest, local_state() could not be initialzed.
    CHECK_IS_TEST();
    return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }

  BraveDarkModeType type = static_cast<BraveDarkModeType>(
      g_browser_process->local_state()->GetInteger(kBraveDarkMode));
  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    if (!SystemDarkModeEnabled()) {
      return GetDarkModeTypeBasedOnChannel();
    }

    return ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
               ? BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK
               : BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }
  return type;
}

BraveDarkModeType GetBraveDarkModeType() {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDarkMode)) {
    return GetDarkModeSwitchValue(command_line);
  }

  if (!g_browser_process || !g_browser_process->local_state()) {
    // In unittest, local_state() could not be initialzed.
    CHECK_IS_TEST();
    return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }

  BraveDarkModeType type = static_cast<BraveDarkModeType>(
      g_browser_process->local_state()->GetInteger(kBraveDarkMode));
  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    if (!SystemDarkModeEnabled()) {
      return GetDarkModeTypeBasedOnChannel();
    }
    return type;
  }
  return type;
}

base::Value::List GetBraveDarkModeTypeList() {
  base::Value::List list;

  if (SystemDarkModeEnabled()) {
    base::Value::Dict system_type;
    system_type.Set(
        "value",
        static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT));
    system_type.Set("name",
                    l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_SYSTEM));
    list.Append(std::move(system_type));
  }

  base::Value::Dict dark_type;
  dark_type.Set("value",
                static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK));
  dark_type.Set("name", l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_DARK));
  list.Append(std::move(dark_type));

  base::Value::Dict light_type;
  light_type.Set(
      "value", static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT));
  light_type.Set("name", l10n_util::GetStringUTF16(IDS_BRAVE_THEME_TYPE_LIGHT));
  list.Append(std::move(light_type));

  return list;
}

}  // namespace dark_mode

// Processes browser-wide theme command line switches.
// This should be called once during browser startup.
void ProcessBrowserWideThemeCommandLineSwitches() {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
  ProcessBrowserWideThemeCommandLineSwitches(
      base::CommandLine::ForCurrentProcess());
#endif
}

// Processes browser-wide theme command line switches with specific command
// line. Used for both startup and running instance scenarios.
void ProcessBrowserWideThemeCommandLineSwitches(
    const base::CommandLine* command_line) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
  ProcessBrowserWideThemeCommandLineSwitches(command_line, nullptr);
#endif
}

// Processes browser-wide theme command line switches with specific command line
// and optional single profile. If single_profile is provided (test scenario),
// only that profile is affected. Otherwise, all loaded profiles are affected.
void ProcessBrowserWideThemeCommandLineSwitches(
    const base::CommandLine* command_line,
    Profile* single_profile) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
  if (!command_line) {
    return;
  }

  // kSetDefaultTheme is browser-wide and affects all profiles
  if (command_line->HasSwitch(brave::themes::switches::kSetDefaultTheme)) {
    if (single_profile) {
      // Test scenario - just affect the single test profile
      ThemeService* theme_service =
          ThemeServiceFactory::GetForProfile(single_profile);
      if (theme_service) {
        theme_service->UseDefaultTheme();
      }
    } else {
      // Production scenario - affect all loaded profiles
      if (!g_browser_process || !g_browser_process->profile_manager()) {
        return;
      }

      ProfileManager* profile_manager = g_browser_process->profile_manager();
      for (Profile* profile : profile_manager->GetLoadedProfiles()) {
        if (profile) {
          ThemeService* theme_service =
              ThemeServiceFactory::GetForProfile(profile);
          if (theme_service) {
            theme_service->UseDefaultTheme();
          }
        }
      }
    }
  }
#endif
}

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "chrome/browser/profiles/profile.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/mojom/themes.mojom.h"

namespace {

// Processes per-profile theme command line switches and applies them to the
// ThemeService. These switches are per-profile:
// - kSetUserColor: Sets the seed color for Material You dynamic theming (GM3)
// - kSetColorScheme: Controls light/dark mode preference
// - kSetColorVariant: Sets Material You color variant (tonal_spot, neutral,
// vibrant, expressive)
// - kSetGrayscaleTheme: Enables grayscale overlay (boolean flag - presence =
// true)
//
// Note: kSetDefaultTheme is processed browser-wide, not here.
void ProcessThemeCommandLineSwitches(const base::CommandLine* command_line,
                                     ThemeService* theme_service) {
  if (!command_line || !theme_service) {
    return;
  }

  if (command_line->HasSwitch(brave::themes::switches::kSetUserColor)) {
    std::string value = command_line->GetSwitchValueASCII(brave::themes::switches::kSetUserColor);
    std::vector<std::string> rgb = base::SplitString(
        value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (rgb.size() == 3) {
      int r_int, g_int, b_int;
      uint8_t r, g, b;
      if (base::StringToInt(rgb[0], &r_int) &&
          base::StringToInt(rgb[1], &g_int) &&
          base::StringToInt(rgb[2], &b_int) &&
          base::CheckedNumeric<uint8_t>(r_int).AssignIfValid(&r) &&
          base::CheckedNumeric<uint8_t>(g_int).AssignIfValid(&g) &&
          base::CheckedNumeric<uint8_t>(b_int).AssignIfValid(&b)) {
        theme_service->SetUserColor(SkColorSetRGB(r, g, b));
      }
    }
  }

  if (command_line->HasSwitch(brave::themes::switches::kSetColorScheme)) {
    std::string scheme = command_line->GetSwitchValueASCII(brave::themes::switches::kSetColorScheme);
    static constexpr auto kSchemeMap =
        base::MakeFixedFlatMap<std::string_view,
                               ThemeService::BrowserColorScheme>({
            {"system", ThemeService::BrowserColorScheme::kSystem},
            {"light", ThemeService::BrowserColorScheme::kLight},
            {"dark", ThemeService::BrowserColorScheme::kDark},
        });

    const auto* color_scheme = base::FindOrNull(kSchemeMap, scheme);
    if (color_scheme) {
      theme_service->SetBrowserColorScheme(*color_scheme);
    }
  }

  if (command_line->HasSwitch(brave::themes::switches::kSetGrayscaleTheme)) {
    theme_service->SetIsGrayscale(true);
  }

  if (command_line->HasSwitch(brave::themes::switches::kSetColorVariant)) {
    std::string variant = command_line->GetSwitchValueASCII(brave::themes::switches::kSetColorVariant);
    static constexpr auto kVariantMap =
        base::MakeFixedFlatMap<std::string_view,
                               ui::mojom::BrowserColorVariant>({
            {"tonal_spot", ui::mojom::BrowserColorVariant::kTonalSpot},
            {"neutral", ui::mojom::BrowserColorVariant::kNeutral},
            {"vibrant", ui::mojom::BrowserColorVariant::kVibrant},
            {"expressive", ui::mojom::BrowserColorVariant::kExpressive},
        });

    const auto* color_variant = base::FindOrNull(kVariantMap, variant);
    if (color_variant) {
      theme_service->SetBrowserColorVariant(*color_variant);
    }
  }
}

}  // namespace

// Processes theme command line switches for the specified profile.
// Gets the ThemeService for the profile and applies the switches.
void ProcessThemeCommandLineSwitchesForProfile(
    const base::CommandLine* command_line,
    Profile* profile) {
  if (!command_line || !profile) {
    return;
  }

  ThemeService* theme_service = ThemeServiceFactory::GetForProfile(profile);
  if (!theme_service) {
    return;
  }

  ProcessThemeCommandLineSwitches(command_line, theme_service);
}

#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) ||
        // BUILDFLAG(IS_CHROMEOS)
