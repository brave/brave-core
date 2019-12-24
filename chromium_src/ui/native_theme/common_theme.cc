// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/native_theme/common_theme.h"

#define GetAuraColor GetAuraColor_ChromiumImpl
#include "../../../../ui/native_theme/common_theme.cc"
#undef GetAuraColor

namespace {
  const SkColor kBraveColorBrand = SkColorSetRGB(0xfb, 0x54, 0x2b);
  const SkColor kBraveColorOrange300 = SkColorSetRGB(0xFF, 0x97, 0x7D);
}  // namespace


namespace ui {

SkColor GetAuraColor(NativeTheme::ColorId color_id,
                    const NativeTheme* base_theme,
                    NativeTheme::ColorScheme color_scheme) {
  if (color_scheme == NativeTheme::ColorScheme::kDefault)
    color_scheme = base_theme->GetDefaultSystemColorScheme();
  const bool is_dark = (color_scheme == NativeTheme::ColorScheme::kDark);
  switch (color_id) {
    case NativeTheme::kColorId_ButtonEnabledColor:
      return is_dark ? SK_ColorWHITE
                    : SkColorSetRGB(0x3b, 0x3e, 0x4f);
    case NativeTheme::kColorId_ButtonPressedShade:
      return SkColorSetA(kBraveColorBrand, is_dark ? 0x2b : 0x23);
    case NativeTheme::kColorId_ProminentButtonColor:
    case NativeTheme::kColorId_ProminentButtonFocusedColor:
      return kBraveColorBrand;
    case NativeTheme::kColorId_ProminentButtonDisabledColor:
      return gfx::kGoogleGrey800;
    case NativeTheme::kColorId_TextOnProminentButtonColor:
      return SK_ColorWHITE;
    case NativeTheme::kColorId_ButtonBorderColor:
      return SkColorSetRGB(0xc2, 0xc4, 0xcf);
    case NativeTheme::kColorId_LabelEnabledColor: {
      SkColor button = GetAuraColor(NativeTheme::kColorId_ButtonEnabledColor,
          base_theme);
      return button;
    }
    case NativeTheme::kColorId_LinkEnabled:
    case NativeTheme::kColorId_LinkPressed:
      return is_dark ? kBraveColorOrange300 : kBraveColorBrand;
    default:
      break;
  }
  return GetAuraColor_ChromiumImpl(color_id, base_theme, color_scheme);
}

}  // namespace ui

