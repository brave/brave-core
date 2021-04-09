/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils_internal.h"

#include "ui/native_theme/native_theme.h"

// Inserted in the ui namespace to have a friendship with ui::NativeTheme.
// Below two methods call protected methods of ui::NativeTheme.
// They are protected methods that called by platform specific subclasses
// whenever system os theme is changed.
// We also want to call whenever brave theme is changed for updating webui/base
// ui modules theme.
namespace ui {

void SetUseDarkColors(bool dark_mode) {
  NativeTheme::GetInstanceForNativeUi()->set_use_dark_colors(dark_mode);
  NativeTheme::GetInstanceForWeb()->set_use_dark_colors(dark_mode);
}
// Recalculate preferred color scheme based on current dark mode that set by
// SetDarkMode() and set it to NativeTheme.
void ReCalcAndSetPreferredColorScheme() {
  auto scheme =
      NativeTheme::GetInstanceForNativeUi()->CalculatePreferredColorScheme();
  NativeTheme::GetInstanceForNativeUi()->set_preferred_color_scheme(scheme);
  NativeTheme::GetInstanceForWeb()->set_preferred_color_scheme(scheme);
}

}  // namespace ui

namespace dark_mode {

namespace internal {

void SetSystemDarkModeForNonDefaultMode(bool dark_mode) {
  // Call SetUseDarkColors() first then call ReCalcPreferredColorScheme()
  // because ReCalcPreferredColorScheme() calculates preferred color scheme
  // based on dark mode.
  ui::SetUseDarkColors(dark_mode);
  ui::ReCalcAndSetPreferredColorScheme();
  // Have to notify observers explicitly because ui::SetDarkMode() and
  // ui::ReCalcPreferredColorScheme| just update ui::NativeTheme:dark_mode_ and
  // ui::NativeTheme:preferred_color_scheme_ values. Need to propagate them.
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
  ui::NativeTheme::GetInstanceForWeb()->NotifyOnNativeThemeUpdated();
}

}  // namespace internal

}  // namespace dark_mode
