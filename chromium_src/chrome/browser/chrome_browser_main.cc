/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/themes/theme_command_line_handler.h"

#include "base/command_line.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/chrome_switches.h"
#include "ui/base/mojom/themes.mojom.h"

namespace {

// Processes theme command line switches and applies them to the ThemeService.
// These switches work together:
// - kSetDefaultTheme: Resets to system default (takes precedence, returns early)
// - kSetUserColor: Sets the seed color for Material You dynamic theming (GM3)
// - kSetColorScheme: Controls light/dark mode preference
// - kSetColorVariant: Sets Material You color variant (tonal_spot, neutral, vibrant, expressive)
// - kSetGrayscaleTheme: Enables/disables grayscale overlay
//
// They can be combined, e.g.:
// --set-user-color="100,150,200" --set-color-scheme="dark" --set-color-variant="vibrant"
void ProcessThemeCommandLineSwitches(const base::CommandLine* command_line,
                                     ThemeService* theme_service) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
    if (!command_line || !theme_service) {
      return;
    }

    if (command_line->HasSwitch(kSetDefaultTheme)) {
      theme_service->UseDefaultTheme();
      return;
    }

    if (command_line->HasSwitch(kSetUserColor)) {
      std::string value =
          command_line->GetSwitchValueASCII(kSetUserColor);
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

    if (command_line->HasSwitch(kSetColorScheme)) {
      std::string scheme =
          command_line->GetSwitchValueASCII(kSetColorScheme);
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

    if (command_line->HasSwitch(kSetGrayscaleTheme)) {
      std::string value =
          command_line->GetSwitchValueASCII(kSetGrayscaleTheme);
      bool is_grayscale = (value == "true");
      theme_service->SetIsGrayscale(is_grayscale);
    }

    if (command_line->HasSwitch(kSetColorVariant)) {
      std::string variant =
          command_line->GetSwitchValueASCII(kSetColorVariant);
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
#endif
  }

}  // namespace

namespace brave {
namespace themes {

// Processes theme command line switches for the specified profile.
// Gets the ThemeService for the profile and applies the switches.
void ProcessThemeCommandLineSwitchesForProfile(
    const base::CommandLine* command_line,
    Profile* profile) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
    if (!command_line || !profile) {
      return;
    }

    ThemeService* theme_service = ThemeServiceFactory::GetForProfile(profile);
    if (!theme_service) {
      return;
    }

    ProcessThemeCommandLineSwitches(command_line, theme_service);
#endif
}

}  // namespace themes
}  // namespace brave

// Macro injected into ProcessSingletonNotificationCallbackImpl to handle
// theme switches when Chrome is already running and receives new command line args.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL \
  ProfileManager* profile_manager = g_browser_process->profile_manager(); \
  if (profile_manager) { \
    Profile* profile = \
        profile_manager->GetProfileByPath(startup_profile_path_info.path); \
    if (profile) { \
      brave::themes::ProcessThemeCommandLineSwitchesForProfile(&command_line, profile); \
    } \
  }
#else
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
#endif  // #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

// Macro injected into PostProfileInit to handle theme switches during
// initial profile setup after Chrome startup.
#if !BUILDFLAG(IS_ANDROID)
#define BRAVE_POST_PROFILE_INIT \
  brave::themes::ProcessThemeCommandLineSwitchesForProfile( \
      base::CommandLine::ForCurrentProcess(), profile);
#else
#define BRAVE_POST_PROFILE_INIT
#endif  // !BUILDFLAG(IS_ANDROID)

#define BrowserProcessImpl BraveBrowserProcessImpl
#define ChromeBrowserMainParts ChromeBrowserMainParts_ChromiumImpl
#include <chrome/browser/chrome_browser_main.cc>
#undef ChromeBrowserMainParts
#undef BrowserProcessImpl
#undef BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
#undef BRAVE_POST_PROFILE_INIT
