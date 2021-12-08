// Copyright (c) 2020 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "ui/native_theme/native_theme.h"

#include "ui/gfx/color_utils.h"

#define GetSystemButtonPressedColor GetSystemButtonPressedColor_ChromiumImpl
#include "src/ui/native_theme/native_theme.cc"
#undef GetSystemButtonPressedColor

namespace ui {

SkColor NativeTheme::GetSystemButtonPressedColor(SkColor base_color) const {
  const SkColor kBraveColorBrand = SkColorSetRGB(0xfb, 0x54, 0x2b);
  bool is_dark = (GetPreferredColorScheme() == PreferredColorScheme::kDark);
  return color_utils::GetResultingPaintColor(
      SkColorSetA(kBraveColorBrand, is_dark ? 0x2b : 0x23), base_color);
}

}  // namespace ui
