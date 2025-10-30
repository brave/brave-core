/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_dark_mode_manager_linux.h"

#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "ui/linux/linux_ui_factory.h"

namespace ui {

BraveDarkModeManagerLinux::BraveDarkModeManagerLinux()
    : DarkModeManagerLinux() {
  // In base class' ctor, |preferred_color_scheme_| is set by calling
  // SetColorScheme() when ui::GetDefaultLinuxUiTheme()
  if (ui::GetDefaultLinuxUiTheme()) {
    dark_mode::CacheSystemDarkModePrefs(preferred_color_scheme_);
  }
}

BraveDarkModeManagerLinux::~BraveDarkModeManagerLinux() = default;

void BraveDarkModeManagerLinux::SetColorScheme(
    NativeTheme::PreferredColorScheme color_scheme,
    bool from_toolkit_theme) {
  dark_mode::CacheSystemDarkModePrefs(color_scheme);
  if (dark_mode::GetBraveDarkModeType() ==
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    DarkModeManagerLinux::SetColorScheme(color_scheme, from_toolkit_theme);
  } else {
    // Make |preferred_color_scheme_| stores latest system theme even brave
    // theme( dark or light) is set. If not, system theme change could not be
    // applied properly later.
    preferred_color_scheme_ = color_scheme;
  }
}

}  // namespace ui
