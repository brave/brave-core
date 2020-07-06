// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_VIEWS_PROPORTIONAL_IMAGE_VIEW_H_

#include "base/macros.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/views/view.h"

namespace brave_custom_notification {

// ProportionalImageViews scale and center their images while preserving their
// original proportions.
class ProportionalImageView : public views::View {
 public:
  // Internal class name.
  static const char kViewClassName[];

  explicit ProportionalImageView(const gfx::Size& view_size);
  ~ProportionalImageView() override;

  // |image| is scaled to fit within |view_size| and |max_image_size| while
  // maintaining its original aspect ratio. It is then centered within the view.
  void SetImage(const gfx::ImageSkia& image,
                const gfx::Size& max_image_size);

  const gfx::ImageSkia& image() const { return image_; }

  // Overridden from views::View:
  void OnPaint(gfx::Canvas* canvas) override;
  const char* GetClassName() const override;

 private:
  gfx::Size GetImageSizeForContainerSize(const gfx::Size& container_size,
                                       const gfx::Size& image_size);
  gfx::Size GetImageDrawingSize();

  gfx::ImageSkia image_;
  gfx::Size max_image_size_;

  DISALLOW_COPY_AND_ASSIGN(ProportionalImageView);
};

}

#endif
