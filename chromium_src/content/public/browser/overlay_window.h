/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_OVERLAY_WINDOW_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_OVERLAY_WINDOW_H_

#include <optional>

namespace media_session {
struct MediaPosition;
}  // namespace media_session

#define SetPlaybackState                                                    \
  SetMediaPosition(                                                         \
      const std::optional<media_session::MediaPosition>& media_position) {} \
  virtual void SetSeekerEnabled(bool enabled) {}                            \
  virtual void SetPlaybackState

#include "src/content/public/browser/overlay_window.h"  // IWYU pragma: export

#undef SetPlaybackState

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_OVERLAY_WINDOW_H_
