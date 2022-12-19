/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LAYOUT_LINUX_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LAYOUT_LINUX_H_

#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"

#define GetInputInsets                                  \
  Unused() { return {}; }                               \
  int NonClientTopHeight(bool restored) const override; \
  gfx::Insets GetInputInsets

#include "src/chrome/browser/ui/views/frame/browser_frame_view_layout_linux.h"

#undef GetInputInsets

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LAYOUT_LINUX_H_
