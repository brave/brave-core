/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.h"

#if defined(USE_DBUS)
#include "chrome/browser/ui/views/dark_mode_manager_linux.h"
#endif

#if defined(USE_DBUS)
namespace ui {

class BraveDarkModeManagerLinux {
 public:
  BraveDarkModeManagerLinux() = default;
  ~BraveDarkModeManagerLinux() = default;
  BraveDarkModeManagerLinux(const BraveDarkModeManagerLinux&) = delete;
  BraveDarkModeManagerLinux& operator=(const BraveDarkModeManagerLinux&) =
      delete;
};

}  // namespace ui

#define DarkModeManagerLinux BraveDarkModeManagerLinux
#endif

#include "src/chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.cc"

#if defined(USE_DBUS)
#undef DarkModeManagerLinux
#endif
