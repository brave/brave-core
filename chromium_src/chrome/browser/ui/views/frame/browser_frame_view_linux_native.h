/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_

#define MaybeUpdateCachedFrameButtonImages       \
  MaybeUpdateCachedFrameButtonImages_Unused() {} \
  friend class BraveBrowserFrameViewLinuxNative; \
  virtual void MaybeUpdateCachedFrameButtonImages

#include "src/chrome/browser/ui/views/frame/browser_frame_view_linux_native.h"  // IWYU pragma: export

#undef MaybeUpdateCachedFrameButtonImages

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_FRAME_VIEW_LINUX_NATIVE_H_
