/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_mac.h"

namespace ui {
bool ShouldOverride(NativeTheme::ColorId color_id) {
  return color_id == NativeTheme::kColorId_FocusedBorderColor;
}

#define GET_BRAVE_COLOR(color_id)                                          \
  if (ShouldOverride(color_id)) {                                          \
    return GetAuraColor(color_id, NativeTheme::GetInstanceForNativeUi());  \
  }

}  // namespace ui

#define GetSystemButtonPressedColor GetSystemButtonPressedColor_ChromiumImpl
#include "src/ui/native_theme/native_theme_mac.mm"
#undef GetSystemButtonPressedColor
#undef GET_BRAVE_COLOR

namespace ui {

SkColor NativeThemeMac::GetSystemButtonPressedColor(SkColor base_color) const {
  return NativeTheme::GetSystemButtonPressedColor(base_color);
}

}  // namespace ui
