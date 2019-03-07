/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/native_theme/native_theme_dark_aura.h"

void NotifyProperThemeObserver();

#include "../../../../ui/native_theme/native_theme_win.cc"  // NOLINT

// TODO(simonhong): Move this function to ui namespace to share with
// native_theme_mac.mm.
void NotifyProperThemeObserver() {
  // When theme is changed from light to dark, we notify to light theme observer
  // because NativeThemeObserver observes light native theme
  ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeEnabled()
      ? ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers()
      : ui::NativeThemeDarkAura::instance()->NotifyObservers();
}
