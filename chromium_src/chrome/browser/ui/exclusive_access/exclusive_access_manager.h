// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_MANAGER_H_

// Add const version of fullscreen_controller()
#define fullscreen_controller()     \
  fullscreen_controller() {         \
    return &fullscreen_controller_; \
  }                                 \
  const FullscreenController* fullscreen_controller() const

#include <chrome/browser/ui/exclusive_access/exclusive_access_manager.h>  // IWYU pragma: export

#undef fullscreen_controller

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_EXCLUSIVE_ACCESS_EXCLUSIVE_ACCESS_MANAGER_H_
