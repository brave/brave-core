/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_mac.h"

namespace ui {
bool ShouldOverride(NativeTheme::ColorId color_id) {
  // Always theme for these colors:
  switch (color_id) {
    case NativeTheme::kColorId_FocusedBorderColor:
      return true;
    default:
      break;
  }
  // The rest of these overrides only targets for old macOS like high sierra
  // that doesn't support dark mode. We are using dark mode on old macOS but
  // some below colors are fetched from system color and they are not dark mode
  // aware. So, we should replace those colors with dark mode aware aura color.
  if (@available(macOS 10.14, *)) {
    return false;
  }
  switch (color_id) {
    // case NativeTheme::kColorId_EnabledMenuItemForegroundColor:
    // case NativeTheme::kColorId_DisabledMenuItemForegroundColor:
    // case NativeTheme::kColorId_LabelTextSelectionBackgroundFocused:
    // case NativeTheme::kColorId_TextfieldSelectionBackgroundFocused:
    case NativeTheme::kColorId_FocusedBorderColor:
      return true;
    default:
      break;
  }
  return false;
}

#define GET_BRAVE_COLOR(color_id)                                          \
  if (ShouldOverride(color_id)) {                                          \
    return GetAuraColor(color_id, NativeTheme::GetInstanceForNativeUi());  \
  }

}  // namespace ui

#define GetSystemButtonPressedColor GetSystemButtonPressedColor_ChromiumImpl
#include "../../../../ui/native_theme/native_theme_mac.mm"
#undef GetSystemButtonPressedColor

namespace ui {

SkColor NativeThemeMac::GetSystemButtonPressedColor(SkColor base_color) const {
  return NativeTheme::GetSystemButtonPressedColor(base_color);
}

}  // namespace ui
