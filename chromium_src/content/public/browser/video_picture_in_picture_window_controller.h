/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_

#include "base/time/time.h"

#define NextSlide        \
  RequestFullscreen() {} \
  virtual void NextSlide

#include "src/content/public/browser/video_picture_in_picture_window_controller.h"  // IWYU pragma: export

#undef NextSlide

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_H_
