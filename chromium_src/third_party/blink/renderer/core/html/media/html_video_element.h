/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_

#include "third_party/blink/renderer/core/html/media/html_media_element.h"

#define RequestEnterPictureInPicture \
  RequestFullscreen() final;         \
  void RequestEnterPictureInPicture

#include "src/third_party/blink/renderer/core/html/media/html_video_element.h"  // IWYU pragma: export

#undef RequestEnterPictureInPicture

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_HTML_VIDEO_ELEMENT_H_
