/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_PUBLIC_BROWSER_YOUTUBE_FULLSCREEN_PAGE_DATA_H_
#define BRAVE_CONTENT_PUBLIC_BROWSER_YOUTUBE_FULLSCREEN_PAGE_DATA_H_

#include "base/supports_user_data.h"
#include "content/common/content_export.h"

namespace content {

// Shared data structure for tracking YouTube fullscreen state
struct CONTENT_EXPORT YouTubeFullscreenPageData : public base::SupportsUserData::Data {
 public:
  explicit YouTubeFullscreenPageData(bool fullscreen_requested = false)
      : fullscreen_requested_(fullscreen_requested) {}

  bool fullscreen_requested() const { return fullscreen_requested_; }
  void set_fullscreen_requested(bool requested) {
    fullscreen_requested_ = requested;
  }

 private:
  bool fullscreen_requested_;
};

// Key for storing YouTubeFullscreenPageData in NavigationEntry UserData
CONTENT_EXPORT extern const char kYouTubeFullscreenPageDataKey[];

}  // namespace content

#endif  // BRAVE_CONTENT_PUBLIC_BROWSER_YOUTUBE_FULLSCREEN_PAGE_DATA_H_