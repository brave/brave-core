/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_custom_notification/proportional_image_view.h"

#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace brave_custom_notification {

const char ProportionalImageView::kViewClassName[] = "ProportionalImageView";

ProportionalImageView::ProportionalImageView(const gfx::Size& view_size) {
  SetPreferredSize(view_size);
}

ProportionalImageView::~ProportionalImageView() {}

void ProportionalImageView::SetImage(const gfx::ImageSkia& image,
                                     const gfx::Size& max_image_size) {
  image_ = image;
  max_image_size_ = max_image_size;
  SchedulePaint();
}

void ProportionalImageView::OnPaint(gfx::Canvas* canvas) {
  views::View::OnPaint(canvas);

  gfx::Size draw_size = GetImageDrawingSize();
  if (draw_size.IsEmpty())
    return;

  gfx::Rect draw_bounds = GetContentsBounds();
  draw_bounds.ClampToCenteredSize(draw_size);

  gfx::ImageSkia image =
      (image_.size() == draw_size)
          ? image_
          : gfx::ImageSkiaOperations::CreateResizedImage(
                image_, skia::ImageOperations::RESIZE_BEST, draw_size);
  canvas->DrawImageInt(image, draw_bounds.x(), draw_bounds.y());
}

const char* ProportionalImageView::GetClassName() const {
  return kViewClassName;
}

gfx::Size ProportionalImageView::GetImageSizeForContainerSize(const gfx::Size& container_size,
                                       const gfx::Size& image_size) {
  if (container_size.IsEmpty() || image_size.IsEmpty())
    return gfx::Size();

  gfx::Size scaled_size = image_size;
  double proportion =
      scaled_size.height() / static_cast<double>(scaled_size.width());
  // We never want to return an empty image given a non-empty container and
  // image, so round the height to 1.
  scaled_size.SetSize(container_size.width(),
                      std::max(0.5 + container_size.width() * proportion, 1.0));
  if (scaled_size.height() > container_size.height()) {
    scaled_size.SetSize(
        std::max(0.5 + container_size.height() / proportion, 1.0),
        container_size.height());
  }

  return scaled_size;
}

gfx::Size ProportionalImageView::GetImageDrawingSize() {
  if (!GetVisible())
    return gfx::Size();

  gfx::Size max_size = max_image_size_;
  max_size.SetToMin(GetContentsBounds().size());
  return GetImageSizeForContainerSize(max_size, image_.size());
}

}  // namespace brave_custom_notification
