/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

#define TabDragController TabDragControllerChromium
#include "src/chrome/browser/ui/tabs/tab_strip_model.cc"  // IWYU pragma: export
#undef TabDragController
