/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/playlist/thumbnail_view.h"

#include <memory>

#include "base/auto_reset.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/skia_paint_util.h"
#include "ui/views/background.h"

namespace {

////////////////////////////////////////////////////////////////////////////////
// DefaultThumbnailBackground
//
// The default thumbnail could be used in various ratio. So we use static image
// for foreground and draw background programmatically.
class DefaultThumbnailBackground : public views::Background {
 public:
  using Background::Background;
  ~DefaultThumbnailBackground() override = default;

  // views::Background:
  void Paint(gfx::Canvas* canvas, views::View* view) const override {
    const auto& bounds = view->GetContentsBounds();
    cc::PaintFlags flags;
    flags.setBlendMode(SkBlendMode::kSrcOver);
    flags.setShader(
        gfx::CreateGradientShader(bounds.bottom_right(), bounds.origin(),
                                  /*start=*/SkColorSetRGB(0x32, 0x2F, 0xB4),
                                  /*end*/ SkColorSetRGB(0x38, 0x35, 0xCA)));

    canvas->DrawRect(bounds, flags);
  }
};

}  // namespace

ThumbnailView::ThumbnailView(const gfx::Image& thumbnail) {
  SetThumbnail(thumbnail);
  SetHorizontalAlignment(views::ImageViewBase::Alignment::kCenter);
  SetVerticalAlignment(views::ImageViewBase::Alignment::kCenter);
}

ThumbnailView::~ThumbnailView() = default;

base::OnceCallback<void(const gfx::Image&)>
ThumbnailView::GetThumbnailSetter() {
  return base::BindOnce(&ThumbnailView::SetThumbnail,
                        weak_ptr_factory_.GetWeakPtr());
}

void ThumbnailView::UpdateImageSize() {
  if (is_updating_image_size_) {
    return;
  }

  // Resize image as much as it can cover this view while keeping the
  // original image ratio.
  base::AutoReset updating_image_size(&is_updating_image_size_, true);
  auto image_size = GetImageModel().Size();
  if (image_size.IsEmpty()) {
    SetImageSize(image_size);
    return;
  }

  const auto preferred_size = GetPreferredSize();

  if (preferred_size.IsEmpty()) {
    SetImageSize(image_size);
  }

  const bool resize_on_horizontal_axis =
      std::abs(preferred_size.width() - image_size.width()) >
      std::abs(preferred_size.height() - image_size.height());
  const float resize_ratio =
      resize_on_horizontal_axis
          ? preferred_size.height() / static_cast<float>(image_size.height())
          : preferred_size.width() / static_cast<float>(image_size.width());
  SetImageSize(gfx::ScaleToCeiledSize(image_size, resize_ratio));
}

void ThumbnailView::PreferredSizeChanged() {
  ImageView::PreferredSizeChanged();
  UpdateImageSize();
}

void ThumbnailView::SetThumbnail(const gfx::Image& thumbnail) {
  if (thumbnail.IsEmpty()) {
    SetImage(ui::ImageModel::FromResourceId(IDR_PLAYLIST_DEFAULT_THUMBNAIL));
    SetBackground(std::make_unique<DefaultThumbnailBackground>());
  } else {
    SetImage(ui::ImageModel::FromImage(thumbnail));
  }

  UpdateImageSize();
}

BEGIN_METADATA(ThumbnailView, views::ImageView)
END_METADATA
