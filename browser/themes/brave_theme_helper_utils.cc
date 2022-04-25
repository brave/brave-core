/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_theme_helper_utils.h"

#include "base/numerics/safe_conversions.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/omnibox/omnibox_theme.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

namespace {

// Location bar colors
const SkColor kPrivateLocationBarBgBase = SkColorSetRGB(0x0B, 0x07, 0x24);
const SkColor kDarkLocationBarBgBase = SkColorSetRGB(0x18, 0x1A, 0x21);
const SkColor kDarkLocationBarHoverBg = SkColorSetRGB(0x23, 0x25, 0x2F);

}  // namespace

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
      color, base::ClampRound(GetOmniboxStateOpacity(state) * 0xff));
}
