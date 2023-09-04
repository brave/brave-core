/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_

#include "ui/views/controls/image_view.h"

class ThumbnailView : public views::ImageView {
 public:
  METADATA_HEADER(ThumbnailView);

  explicit ThumbnailView(const gfx::Image& thumbnail);
  ~ThumbnailView() override;

  void UpdateImageSize();

  // views::ImageView:
  void PreferredSizeChanged() override;

 private:
  bool is_updating_image_size_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_
