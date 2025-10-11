/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

#include "base/check.h"
#include "ui/native_theme/os_settings_provider.h"

namespace dark_mode {

void SetSystemDarkMode(BraveDarkModeType type) {
  ui::NativeTheme::PreferredColorScheme scheme =
      ui::NativeTheme::PreferredColorScheme::kNoPreference;

  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    CHECK(ui::OsSettingsProvider::Get().DarkColorSchemeAvailable());
  } else {
    scheme = (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK)
                 ? ui::NativeTheme::PreferredColorScheme::kDark
                 : ui::NativeTheme::PreferredColorScheme::kLight;
  }

  ui::NativeTheme::GetInstanceForNativeUi()->set_preferred_color_scheme(scheme);
  ui::NativeTheme::GetInstanceForNativeUi()->NotifyOnNativeThemeUpdated();
}

}  // namespace dark_mode
