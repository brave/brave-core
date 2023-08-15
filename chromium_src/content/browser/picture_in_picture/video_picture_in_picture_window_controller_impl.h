// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_

namespace content {
class BraveVideoPictureInPictureWindowControllerImpl;
}  // namespace content

#define UpdatePlaybackState                                    \
  UpdatePlaybackState();                                       \
  friend class BraveVideoPictureInPictureWindowControllerImpl; \
  void dummy
#include "src/content/browser/picture_in_picture/video_picture_in_picture_window_controller_impl.h"  // IWYU pragma: export
#undef UpdatePlaybackState

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PICTURE_IN_PICTURE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
