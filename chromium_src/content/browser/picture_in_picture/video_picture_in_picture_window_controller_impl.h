/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_

#include "content/public/browser/video_picture_in_picture_window_controller.h"
#include "services/media_session/public/mojom/media_session.mojom.h"

#define NextSlide               \
  RequestFullscreen() override; \
  void NextSlide
#define media_session_action_next_slide_handled_    \
  media_session_action_next_slide_handled_ = false; \
  bool media_session_action_seek_to_handled_

#include "src/content/browser/picture_in_picture/video_picture_in_picture_window_controller_impl.h"  // IWYU pragma: export

#undef media_session_action_next_slide_handled_
#undef NextSlide

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
