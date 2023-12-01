/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

#include <optional>

#include "brave/browser/themes/brave_dark_mode_utils_internal.h"
#include "ui/native_theme/native_theme.h"

namespace dark_mode {

namespace {
std::optional<bool> g_system_dark_mode_prefs;
}  // namespace

void CacheSystemDarkModePrefs(bool prefer_dark_theme) {
  g_system_dark_mode_prefs = prefer_dark_theme;
}

bool HasCachedSystemDarkModeType() {
  return g_system_dark_mode_prefs.has_value();
}

void SetSystemDarkMode(BraveDarkModeType type) {
  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    if (g_system_dark_mode_prefs.has_value()) {
      internal::SetSystemDarkModeForNonDefaultMode(*g_system_dark_mode_prefs);
    }
    return;
  }

  internal::SetSystemDarkModeForNonDefaultMode(
      type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK);
}

}  // namespace dark_mode
