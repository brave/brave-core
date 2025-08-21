/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/chrome_browser_main.h"
#include "brave/browser/brave_browser_process_impl.h"

#include "base/command_line.h"
#include "base/containers/fixed_flat_map.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/common/chrome_switches.h"
#include "ui/base/mojom/themes.mojom.h"

namespace {

class ThemeCommandLineHandler {
 public:
  static void ProcessThemeCommandLineSwitches(
      const base::CommandLine* command_line,
      ThemeService* theme_service) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || \
    BUILDFLAG(IS_CHROMEOS)
    if (!command_line || !theme_service) {
      return;
    }

    if (command_line->HasSwitch(switches::kSetDefaultTheme)) {
      theme_service->UseDefaultTheme();
      return;
    }

    if (command_line->HasSwitch(switches::kSetUserColor)) {
      std::string value =
          command_line->GetSwitchValueASCII(switches::kSetUserColor);
      std::vector<std::string> rgb = base::SplitString(
          value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
      if (rgb.size() == 3) {
        int r, g, b;
        if (base::StringToInt(rgb[0], &r) && base::StringToInt(rgb[1], &g) &&
            base::StringToInt(rgb[2], &b) && r >= 0 && r <= 255 && g >= 0 &&
            g <= 255 && b >= 0 && b <= 255) {
          theme_service->SetUserColor(SkColorSetRGB(r, g, b));
        }
      }
    }

    if (command_line->HasSwitch(switches::kSetColorScheme)) {
      std::string scheme =
          command_line->GetSwitchValueASCII(switches::kSetColorScheme);
      static constexpr auto kSchemeMap =
          base::MakeFixedFlatMap<std::string_view,
                                 ThemeService::BrowserColorScheme>({
              {"system", ThemeService::BrowserColorScheme::kSystem},
              {"light", ThemeService::BrowserColorScheme::kLight},
              {"dark", ThemeService::BrowserColorScheme::kDark},
          });

      auto it = kSchemeMap.find(scheme);
      if (it != kSchemeMap.end()) {
        theme_service->SetBrowserColorScheme(it->second);
      }
    }

    if (command_line->HasSwitch(switches::kSetGrayscaleTheme)) {
      std::string value =
          command_line->GetSwitchValueASCII(switches::kSetGrayscaleTheme);
      bool is_grayscale = (value == "true");
      theme_service->SetIsGrayscale(is_grayscale);
    }

    if (command_line->HasSwitch(switches::kSetColorVariant)) {
      std::string variant =
          command_line->GetSwitchValueASCII(switches::kSetColorVariant);
      static constexpr auto kVariantMap =
          base::MakeFixedFlatMap<std::string_view,
                                 ui::mojom::BrowserColorVariant>({
              {"tonal_spot", ui::mojom::BrowserColorVariant::kTonalSpot},
              {"neutral", ui::mojom::BrowserColorVariant::kNeutral},
              {"vibrant", ui::mojom::BrowserColorVariant::kVibrant},
              {"expressive", ui::mojom::BrowserColorVariant::kExpressive},
          });

      auto it = kVariantMap.find(variant);
      if (it != kVariantMap.end()) {
        theme_service->SetBrowserColorVariant(it->second);
      }
    }
#endif
  }

  static void ProcessForProfile(const base::CommandLine* command_line,
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

  ThemeCommandLineHandler() = delete;
  ~ThemeCommandLineHandler() = delete;
};

}  // namespace

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL \
  ProfileManager* profile_manager = g_browser_process->profile_manager(); \
  if (profile_manager) { \
    Profile* profile = \
        profile_manager->GetProfileByPath(startup_profile_path_info.path); \
    if (profile) { \
      ThemeCommandLineHandler::ProcessForProfile(&command_line, profile); \
    } \
  }
#else
#define BRAVE_PROCESS_SINGLETON_NOTIFICATION_CALLBACK_IMPL
#endif  // #if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

#if !BUILDFLAG(IS_ANDROID)
#define BRAVE_POST_PROFILE_INIT \
  ThemeCommandLineHandler::ProcessForProfile( \
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
