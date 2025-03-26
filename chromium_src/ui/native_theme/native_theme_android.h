/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_ANDROID_H_
#define BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_ANDROID_H_

#define GetInstanceForNativeUi \
  GetInstanceForNativeUi();    \
  static ui::NativeTheme* GetInstanceForNativeUi_UnUsed

#include "src/ui/native_theme/native_theme_android.h"  // IWYU pragma: export
#undef GetInstanceForNativeUi

#endif  // BRAVE_CHROMIUM_SRC_UI_NATIVE_THEME_NATIVE_THEME_ANDROID_H_
