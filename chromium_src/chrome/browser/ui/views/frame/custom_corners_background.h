/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CUSTOM_CORNERS_BACKGROUND_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CUSTOM_CORNERS_BACKGROUND_H_

#include "ui/views/view.h"

#define SetVisible(...)        \
  SetVisible(__VA_ARGS__);     \
  Corners GetCorners() const { \
    return corners_;           \
  }                            \
  void SetVisible_Unused(__VA_ARGS__)

#include <chrome/browser/ui/views/frame/custom_corners_background.h>  // IWYU pragma: export

#undef SetVisible

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CUSTOM_CORNERS_BACKGROUND_H_
