// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_
#define BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_

#define GetSystemButtonPressedColor                                   \
  GetSystemButtonPressedColor_ChromiumImpl(SkColor base_color) const; \
  friend void SetUseDarkColors(bool dark_mode);                       \
  friend void ReCalcAndSetPreferredColorScheme();                     \
  static NativeTheme* BraveGetInstanceForNativeUi();                  \
  virtual SkColor GetSystemButtonPressedColor

#include "../../../../ui/native_theme/native_theme.h"
#undef GetSystemButtonPressedColor

#endif  // BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_H_
