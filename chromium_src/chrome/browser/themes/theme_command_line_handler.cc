// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_command_line_handler.h"

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

void ThemeCommandLineHandler::ProcessThemeCommandLineSwitches(
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

void ThemeCommandLineHandler::ProcessForProfile(
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