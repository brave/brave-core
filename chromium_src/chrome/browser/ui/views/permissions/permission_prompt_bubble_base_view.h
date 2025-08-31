// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_

#include "ui/base/ui_base_types.h"

// Add RestoreParentWidgetZOrderLevel() and a member variable in order to reset
// parent widget's z-order level.
#define ShowWidget                              \
  RestoreParentWidgetZOrderLevel();             \
                                                \
 private:                                       \
  ui::ZOrderLevel parent_widget_z_order_level_; \
                                                \
 public:                                        \
  void ShowWidget

#include <chrome/browser/ui/views/permissions/permission_prompt_bubble_base_view.h>  // IWYU pragma: export

#undef ShowWidget

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PERMISSIONS_PERMISSION_PROMPT_BUBBLE_BASE_VIEW_H_
