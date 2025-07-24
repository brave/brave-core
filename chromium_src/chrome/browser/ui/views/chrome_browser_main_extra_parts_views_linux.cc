/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.h"

#if BUILDFLAG(USE_DBUS)
#include "brave/browser/ui/views/brave_dark_mode_manager_linux.h"
#include "chrome/browser/ui/views/dark_mode_manager_linux.h"
#endif

#if BUILDFLAG(USE_DBUS)
#define DarkModeManagerLinux BraveDarkModeManagerLinux
#endif

#include <chrome/browser/ui/views/chrome_browser_main_extra_parts_views_linux.cc>

#if BUILDFLAG(USE_DBUS)
#undef DarkModeManagerLinux
#endif
