/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/omnibox/omnibox_theme.h"

#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/native_theme/native_theme.h"

// Overriden version
SkColor GetOmniboxColor(OmniboxPart part,
                        OmniboxTint tint,
                        OmniboxPartState state);

#include "../../../chrome/browser/ui/omnibox/omnibox_theme.cc"  // NOLINT

namespace {

OmniboxTint BraveTintToChromiumTint(OmniboxTint brave_tint) {
  switch (brave_tint) {
    case OmniboxTint::PRIVATE:
    case OmniboxTint::DARK:
      return OmniboxTint::DARK;
    case OmniboxTint::LIGHT:
      return OmniboxTint::LIGHT;
    case OmniboxTint::NATIVE:
      return OmniboxTint::NATIVE;
    default:
      return OmniboxTint::LIGHT;
  }
}

constexpr SkColor DarkPrivateLight(OmniboxTint tint,
                                   SkColor dark,
                                   SkColor priv,
                                   SkColor light) {
  switch (tint) {
    case OmniboxTint::DARK:
      return dark;
    case OmniboxTint::PRIVATE:
      return priv;
    default:
      return light;
  }
}

const SkColor kPrivateLocationBarBackground = SkColorSetRGB(0x1b, 0x0e, 0x2c);

}  // namespace

// Overriden version
SkColor GetOmniboxColor(OmniboxPart part,
                        OmniboxTint tint,
                        OmniboxPartState state) {
  // Note: OmniboxTint::NATIVE is no longer possible
  const bool dark = tint == OmniboxTint::DARK;
  const bool priv = tint == OmniboxTint::PRIVATE;
  // For high contrast, selected rows use inverted colors to stand out more.
  ui::NativeTheme* native_theme = ui::NativeTheme::GetInstanceForNativeUi();
  bool high_contrast = native_theme && native_theme->UsesHighContrastColors();
  // TODO(petemill): Get colors from color-pallete and theme constants
  switch (part) {
    case OmniboxPart::LOCATION_BAR_BACKGROUND: {
      const bool hovered = state == OmniboxPartState::HOVERED;
      return dark
                 ? (hovered ? SkColorSetRGB(0x44, 0x44, 0x44)
                            : SkColorSetRGB(0x22, 0x22, 0x22))
                 : (priv
                        ? color_utils::HSLShift(kPrivateLocationBarBackground,
                                                {-1, -1, hovered ? 0.54 : 0.52})
                        : (hovered ? color_utils::AlphaBlend(
                                         SK_ColorWHITE,
                                         SkColorSetRGB(0xf3, 0xf3, 0xf3),
                                         0.7f)
                                   : SK_ColorWHITE));
    }
    case OmniboxPart::LOCATION_BAR_TEXT_DEFAULT:
    case OmniboxPart::RESULTS_TEXT_DEFAULT: {
      return (dark || priv) ? SkColorSetRGB(0xff, 0xff, 0xff)
                            : SkColorSetRGB(0x42, 0x42, 0x42);
    }
    case OmniboxPart::RESULTS_BACKGROUND:
      return color_utils::BlendTowardMaxContrast(
          DarkPrivateLight(
              tint,
              high_contrast ? gfx::kGoogleGrey900 : gfx::kGoogleGrey800,
              color_utils::HSLShift(kPrivateLocationBarBackground,
                                    {-1, -1, high_contrast ? 0.45 : 0.56}),
              SK_ColorWHITE),
          gfx::ToRoundedInt(GetOmniboxStateOpacity(state) * 0xff));
    default:
      break;
  }

  // All other values, call original function
  OmniboxTint translate_value = BraveTintToChromiumTint(tint);
  return GetOmniboxColor_ChromiumImpl(part, translate_value, state);
}
