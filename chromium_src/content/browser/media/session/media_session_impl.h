/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_

// Add a new method for video_picture_in_picture_window_controller_impl
#define NotifyMediaSessionMetadataChange        \
  NotifyMediaSessionMetadataChange_Unused();    \
  std::optional<media_session::MediaPosition>   \
  GetMediaPositionFromNormalPlayerIfPossible(); \
  void NotifyMediaSessionMetadataChange

#include "src/content/browser/media/session/media_session_impl.h"  // IWYU pragma: export

#undef NotifyMediaSessionMetadataChange

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_MEDIA_SESSION_MEDIA_SESSION_IMPL_H_
