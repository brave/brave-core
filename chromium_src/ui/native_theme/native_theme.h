/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_
#define BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#define GetInstanceForNativeUi \
  GetInstanceForNativeUi();    \
  static ui::NativeTheme* GetInstanceForNativeUi_UnUsed
#endif  // BUILDFLAG(IS_ANDROID)

#define GetSystemButtonPressedColor                                   \
  GetSystemButtonPressedColor_ChromiumImpl(SkColor base_color) const; \
  friend void SetUseDarkColors(bool dark_mode);                       \
  friend void ReCalcAndSetPreferredColorScheme();                     \
  static NativeTheme* BraveGetInstanceForNativeUi();                  \
  virtual SkColor GetSystemButtonPressedColor

#include "src/ui/native_theme/native_theme.h"  // IWYU pragma: export
#undef GetSystemButtonPressedColor

#if BUILDFLAG(IS_ANDROID)
#undef GetInstanceForNativeUi
#endif  // BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_
