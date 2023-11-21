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
  // In base class' ctor, |prefer_dark_theme_| is set by calling
  // SetColorScheme() when ui::GetDefaultLinuxUiTheme()
  if (ui::GetDefaultLinuxUiTheme()) {
    dark_mode::CacheSystemDarkModePrefs(prefer_dark_theme_);
  }
}

BraveDarkModeManagerLinux::~BraveDarkModeManagerLinux() = default;

void BraveDarkModeManagerLinux::SetColorScheme(bool prefer_dark_theme,
                                               bool from_toolkit_theme) {
  dark_mode::CacheSystemDarkModePrefs(prefer_dark_theme);
  if (dark_mode::GetBraveDarkModeType() ==
      dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    DarkModeManagerLinux::SetColorScheme(prefer_dark_theme, from_toolkit_theme);
  } else {
    // Make |prefer_dark_theme_| stores latest system theme even brave theme(
    // dark or light) is set. If not, system theme change could not be applied
    // properly later.
    prefer_dark_theme_ = prefer_dark_theme;
  }
}

}  // namespace ui
