// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TASK_MANAGER_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TASK_MANAGER_VIEW_H_

#include "ui/menus/simple_menu_model.h"

// Replaces Chromium's MenuClosed implementation with ours
#define MenuClosed(...)           \
  MenuClosed_Unused(__VA_ARGS__); \
  void MenuClosed(__VA_ARGS__)
#include <chrome/browser/ui/views/task_manager_view.h>  // IWYU pragma: export
#undef MenuClosed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TASK_MANAGER_VIEW_H_
