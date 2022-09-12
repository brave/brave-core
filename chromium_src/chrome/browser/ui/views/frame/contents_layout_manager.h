/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CONTENTS_LAYOUT_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CONTENTS_LAYOUT_MANAGER_H_

#define SetContentsResizingStrategy        \
  UnUsed() {}                              \
  friend class BraveContentsLayoutManager; \
  void SetContentsResizingStrategy

#include "src/chrome/browser/ui/views/frame/contents_layout_manager.h"

#undef SetContentsResizingStrategy

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_CONTENTS_LAYOUT_MANAGER_H_
