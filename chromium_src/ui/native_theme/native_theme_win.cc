// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

namespace {
bool s_ignore_system_dark_mode_change = false;
}  // namespace

#define RETURN_IF_BRAVE_SHOULD_IGNORE_SYSTEM_DARK_MODE_CHANGE  \
  if (s_ignore_system_dark_mode_change) { \
    return; \
  }

#include "../../../../ui/native_theme/native_theme_win.cc"  // NOLINT

namespace ui {

void NATIVE_THEME_EXPORT IgnoreSystemDarkModeChange(bool ignore) {
  s_ignore_system_dark_mode_change = ignore;
}

}  // namespace ui
