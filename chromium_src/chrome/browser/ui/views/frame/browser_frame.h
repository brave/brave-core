/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_H_

#define OnMenuClosed              \
  OnMenuClosed_Unused() {}        \
  friend class BraveBrowserFrame; \
  void OnMenuClosed

#include "src/chrome/browser/ui/views/frame/browser_frame.h"  // IWYU pragma: export

#undef OnMenuClosed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_H_
