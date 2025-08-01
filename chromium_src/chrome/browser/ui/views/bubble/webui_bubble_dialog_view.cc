/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/bubble/webui_bubble_dialog_view.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

// In the `WebUIBubbleDialogView` constructor, give the bubble rounded corners.
#define SetLayoutManager(LAYOUT) \
  set_use_round_corners(true);   \
  set_corner_radius(16);         \
  SetLayoutManager(LAYOUT)

#include <chrome/browser/ui/views/bubble/webui_bubble_dialog_view.cc>

#undef SetLayoutManager
