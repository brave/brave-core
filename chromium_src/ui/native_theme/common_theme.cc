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
}  // namespace


namespace ui {

SkColor GetAuraColor(NativeTheme::ColorId color_id,
                    const NativeTheme* base_theme,
                    NativeTheme::ColorScheme color_scheme) {
  if (color_scheme == NativeTheme::ColorScheme::kDefault)
    color_scheme = base_theme->GetDefaultSystemColorScheme();
  switch (color_id) {
    case NativeTheme::kColorId_ProminentButtonColor:
    case NativeTheme::kColorId_FocusedBorderColor:
      return kBraveColorBrand;
    case NativeTheme::kColorId_TextOnProminentButtonColor:
      return SK_ColorWHITE;
    default:
      break;
  }
  return GetAuraColor_ChromiumImpl(color_id, base_theme, color_scheme);
}

}  // namespace ui

