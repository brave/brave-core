/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/native_theme/native_theme.h"
#include "ui/native_theme/native_theme_mac.h"

#define GetSystemButtonPressedColor GetSystemButtonPressedColor_ChromiumImpl
#include <ui/native_theme/native_theme_mac.mm>
#undef GetSystemButtonPressedColor

namespace ui {

// Shared instance for dark UI. This was part of Chromium, but got removed in
// Chromium 141. However, we use it for Private/Tor windows.
NativeTheme* NativeTheme::GetInstanceForDarkUI() {
  static base::NoDestructor<NativeThemeMac> s_native_theme(
      /*configure_web_instance=*/false, /*should_only_use_dark_colors=*/true);
  return s_native_theme.get();
}

SkColor NativeThemeMac::GetSystemButtonPressedColor(SkColor base_color) const {
  return NativeTheme::GetSystemButtonPressedColor(base_color);
}

}  // namespace ui
