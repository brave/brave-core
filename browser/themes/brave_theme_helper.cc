/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_helper.h"

#include "base/numerics/safe_conversions.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/themes/theme_properties.h"
#include "brave/browser/ui/color/color_palette.h"
#include "brave/components/brave_vpn/buildflags/buildflags.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"

#if BUILDFLAG(IS_LINUX)
#include "chrome/browser/themes/custom_theme_supplier.h"
#endif

namespace {

#if BUILDFLAG(IS_LINUX)
bool IsUsingSystemTheme(const CustomThemeSupplier* theme_supplier) {
  return theme_supplier &&
         theme_supplier->get_theme_type() ==
             ui::ColorProviderManager::ThemeInitializerSupplier::ThemeType::
                 kNativeX11;
}
#endif

}  // namespace

BraveThemeHelper::~BraveThemeHelper() = default;

SkColor BraveThemeHelper::GetDefaultColor(
    int id,
    bool incognito,
    const CustomThemeSupplier* theme_supplier) const {
  const bool is_brave_theme_properties =
      BraveThemeProperties::IsBraveThemeProperties(id);
#if BUILDFLAG(IS_LINUX)
  // IF gtk theme is selected, respect it.
  if (!is_brave_theme_properties && IsUsingSystemTheme(theme_supplier)) {
    return ThemeHelper::GetDefaultColor(id, incognito, theme_supplier);
  }
#endif

  if (!is_brave_theme_properties && theme_supplier)
    return ThemeHelper::GetDefaultColor(id, incognito, theme_supplier);

  // Brave Tor profiles are always 'incognito' (for now)
  if (!incognito && (is_tor_ || is_guest_)) {
    incognito = true;
  }

  switch (id) {
    // Pick most contrast color between our light and dark colors based on
    // current toolbar color.
#if BUILDFLAG(ENABLE_SIDEBAR)
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_DRAG_INDICATOR_COLOR:
    case BraveThemeProperties::COLOR_SIDEBAR_ADD_BUTTON_DISABLED:
    case BraveThemeProperties::COLOR_SIDEBAR_BUTTON_BASE:
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_NORMAL:
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_DISABLED: {
      const auto toolbar_color =
          GetColor(ThemeProperties::COLOR_TOOLBAR, incognito, theme_supplier);
      const auto base_button_color_light = MaybeGetDefaultColorForBraveUi(
          id, incognito, is_tor_,
          dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
      const auto base_button_color_dark = MaybeGetDefaultColorForBraveUi(
          id, incognito, is_tor_,
          dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
      DCHECK(base_button_color_light && base_button_color_dark);
      return color_utils::PickContrastingColor(base_button_color_light.value(),
                                               base_button_color_dark.value(),
                                               toolbar_color);
    }
    case BraveThemeProperties::COLOR_SIDEBAR_ARROW_BACKGROUND_HOVERED:
    case BraveThemeProperties::COLOR_SIDEBAR_ITEM_BACKGROUND_HOVERED: {
      // Copied from chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h
      // to use same hover background with toolbar button.
      constexpr float kToolbarInkDropHighlightVisibleOpacity = 0.08f;
      return SkColorSetA(GetColor(ThemeProperties::COLOR_TOOLBAR_INK_DROP,
                                  incognito, theme_supplier),
                         0xFF * kToolbarInkDropHighlightVisibleOpacity);
    }
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_CONNECTED:
    case BraveThemeProperties::COLOR_BRAVE_VPN_BUTTON_TEXT_DISCONNECTED: {
      const auto toolbar_color =
          GetColor(ThemeProperties::COLOR_TOOLBAR, incognito, theme_supplier);
      const auto color_for_light = MaybeGetDefaultColorForBraveUi(
          id, incognito, is_tor_,
          dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT);
      const auto color_for_dark = MaybeGetDefaultColorForBraveUi(
          id, incognito, is_tor_,
          dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
      DCHECK(color_for_light && color_for_dark);
      return color_utils::PickContrastingColor(
          color_for_light.value(), color_for_dark.value(), toolbar_color);
    }
#endif
    default:
      break;
  }

  const dark_mode::BraveDarkModeType type =
      dark_mode::GetActiveBraveDarkModeType();
  const absl::optional<SkColor> braveColor =
      MaybeGetDefaultColorForBraveUi(id, incognito, is_tor_, type);
  if (braveColor) {
    return braveColor.value();
  }

  // Make sure we fallback to Chrome's dark theme (incognito) for our dark theme
  if (type == dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK) {
    incognito = true;
  }

  DCHECK(!is_brave_theme_properties);
  return ThemeHelper::GetDefaultColor(id, incognito, theme_supplier);
}
