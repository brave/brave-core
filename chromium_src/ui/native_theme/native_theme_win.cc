/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ui/native_theme/native_theme_dark_aura.h"
#include "ui/native_theme/native_theme_win.h"

namespace {

bool system_dark_mode_overridden = false;
bool dark_mode_enabled = false;

// TODO(simonhong): Move this function to ui namespace to share with
// native_theme_mac.mm.
void NotifyProperThemeObserver() {
  // When theme is changed from light to dark, we notify to light theme observer
  // because NativeThemeObserver observes light native theme
  ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeEnabled()
      ? ui::NativeTheme::GetInstanceForNativeUi()->NotifyObservers()
      : ui::NativeThemeDarkAura::instance()->NotifyObservers();
}

bool OverrideSystemDarkMode() {
  return system_dark_mode_overridden;
}

bool GetSystemDarkModeEnabledOverrides() {
  return dark_mode_enabled;
}

}  // namespace

#include "../../../../ui/native_theme/native_theme_win.cc"  // NOLINT

namespace ui {

void SetOverrideSystemDarkMode(bool override,
                               bool enable_dark_mode) {
  system_dark_mode_overridden = override;
  dark_mode_enabled = enable_dark_mode;
}

}  // namespace ui

