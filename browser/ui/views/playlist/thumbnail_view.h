/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_

#include "base/memory/weak_ptr.h"
#include "ui/views/controls/image_view.h"

class ThumbnailView : public views::ImageView {
 public:
  METADATA_HEADER(ThumbnailView);

  explicit ThumbnailView(const gfx::Image& thumbnail);
  ~ThumbnailView() override;

  base::OnceCallback<void(const gfx::Image&)> GetThumbnailSetter();

  void UpdateImageSize();

  // views::ImageView:
  void PreferredSizeChanged() override;

 private:
  void SetThumbnail(const gfx::Image& thumbnail);

  bool is_updating_image_size_ = false;

  base::WeakPtrFactory<ThumbnailView> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PLAYLIST_THUMBNAIL_VIEW_H_
