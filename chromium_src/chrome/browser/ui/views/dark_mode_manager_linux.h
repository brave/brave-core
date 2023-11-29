/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DARK_MODE_MANAGER_LINUX_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DARK_MODE_MANAGER_LINUX_H_

#define SetColorScheme                    \
  SetColorScheme_UnUsed() {}              \
  friend class BraveDarkModeManagerLinux; \
  virtual void SetColorScheme

#include "src/chrome/browser/ui/views/dark_mode_manager_linux.h"  // IWYU pragma: export

#undef SetColorScheme

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DARK_MODE_MANAGER_LINUX_H_
