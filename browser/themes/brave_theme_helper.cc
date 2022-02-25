/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_helper.h"

#include "base/numerics/safe_conversions.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "ui/gfx/color_palette.h"
#include "ui/native_theme/native_theme.h"

#if defined(OS_LINUX)
#include "chrome/browser/themes/custom_theme_supplier.h"
#endif

namespace {

// TODO(simonhong): Get colors from brave's palette.
// Omnibox text colors
const SkColor kDarkOmniboxText = SkColorSetRGB(0xff, 0xff, 0xff);
const SkColor kLightOmniboxText = SkColorSetRGB(0x42, 0x42, 0x42);

// Location bar colors
const SkColor kPrivateLocationBarBgBase = SkColorSetRGB(0x0B, 0x07, 0x24);
const SkColor kDarkLocationBarBgBase = SkColorSetRGB(0x18, 0x1A, 0x21);
const SkColor kDarkLocationBarHoverBg = SkColorSetRGB(0x23, 0x25, 0x2F);

SkColor GetLocationBarBackground(bool dark, bool priv, bool hover) {
  if (priv) {
    return hover ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                         {-1, -1, 0.54})
                 : kPrivateLocationBarBgBase;
  }

  if (dark) {
    return hover ? kDarkLocationBarHoverBg : kDarkLocationBarBgBase;
  }

  return hover ? color_utils::AlphaBlend(SK_ColorWHITE,
                                         SkColorSetRGB(0xf3, 0xf3, 0xf3), 0.7f)
               : SK_ColorWHITE;
}

// Omnibox result bg colors
SkColor GetOmniboxResultBackground(int id, bool dark, bool priv) {
  // For high contrast, selected rows use inverted colors to stand out more.
  ui::NativeTheme* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool high_contrast =
      native_theme && native_theme->UserHasContrastPreference();
  OmniboxPartState state = OmniboxPartState::NORMAL;
  if (id == ThemeProperties::COLOR_OMNIBOX_RESULTS_BG_HOVERED) {
    state = OmniboxPartState::HOVERED;
  } else if (id == ThemeProperties::COLOR_OMNIBOX_RESULTS_BG_SELECTED) {
    state = OmniboxPartState::SELECTED;
  }

  SkColor color;
  if (priv) {
    color = high_contrast ? color_utils::HSLShift(kPrivateLocationBarBgBase,
                                                  {-1, -1, 0.45})
                          : kPrivateLocationBarBgBase;
  } else if (dark) {
    color = high_contrast ? gfx::kGoogleGrey900 : kDarkLocationBarBgBase;
  } else {
    color = SK_ColorWHITE;
  }
  return color_utils::BlendTowardMaxContrast(
      color,
      base::ClampRound(GetOmniboxStateOpacity(state) * 0xff));
}

#if defined(OS_LINUX)
bool IsUsingSystemTheme(const CustomThemeSupplier* theme_supplier) {
  return theme_supplier &&
         theme_supplier->get_theme_type() == CustomThemeSupplier::NATIVE_X11;
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
#if defined(OS_LINUX)
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

absl::optional<SkColor> BraveThemeHelper::GetOmniboxColor(
    int id,
    bool incognito,
    const CustomThemeSupplier* theme_supplier) const {
#if defined(OS_LINUX)
  // If gtk theme is selected, respect it.
  if (IsUsingSystemTheme(theme_supplier)) {
    return ThemeHelper::GetOmniboxColor(id, incognito, theme_supplier);
  }
#endif

  if (theme_supplier)
    return ThemeHelper::GetOmniboxColor(id, incognito, theme_supplier);

  const bool dark = dark_mode::GetActiveBraveDarkModeType() ==
                    dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  incognito = incognito || is_tor_ || is_guest_;
  // TODO(petemill): Get colors from color-pallete and theme constants
  switch (id) {
    case ThemeProperties::COLOR_OMNIBOX_BACKGROUND: {
      return GetLocationBarBackground(dark, incognito, /*hover*/ false);
    }
    case ThemeProperties::COLOR_OMNIBOX_BACKGROUND_HOVERED: {
      return GetLocationBarBackground(dark, incognito, /*hover*/ true);
    }
    case ThemeProperties::COLOR_OMNIBOX_TEXT: {
      return (dark || incognito) ? kDarkOmniboxText : kLightOmniboxText;
    }
    case ThemeProperties::COLOR_OMNIBOX_RESULTS_BG:
    case ThemeProperties::COLOR_OMNIBOX_RESULTS_BG_HOVERED:
    case ThemeProperties::COLOR_OMNIBOX_RESULTS_BG_SELECTED: {
      return GetOmniboxResultBackground(id, dark, incognito);
    }
    default:
      break;
  }

  // All other values, call original function
  return ThemeHelper::GetOmniboxColor(id, incognito, theme_supplier);
}
