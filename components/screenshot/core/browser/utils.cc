// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/screenshot/core/browser/utils.h"

#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"

namespace screenshot {

SkBitmap ScaleDownBitmap(const SkBitmap& bitmap) {
  constexpr int kTargetWidth = 1024;
  constexpr int kTargetHeight = 768;

  if (bitmap.width() <= kTargetWidth && bitmap.height() <= kTargetHeight) {
    return bitmap;
  }

  SkBitmap scaled_bitmap;
  scaled_bitmap.allocN32Pixels(kTargetWidth, kTargetHeight);

  SkCanvas canvas(scaled_bitmap);
  canvas.clear(SK_ColorTRANSPARENT);

  SkSamplingOptions sampling_options(SkFilterMode::kLinear,
                                     SkMipmapMode::kLinear);

  float src_aspect = static_cast<float>(bitmap.width()) / bitmap.height();
  float dst_aspect = static_cast<float>(kTargetWidth) / kTargetHeight;

  SkRect dst_rect;
  if (src_aspect > dst_aspect) {
    // Source is wider — fit to width.
    float scaled_height = kTargetWidth / src_aspect;
    float y_offset = (kTargetHeight - scaled_height) / 2;
    dst_rect = SkRect::MakeXYWH(0, y_offset, kTargetWidth, scaled_height);
  } else {
    // Source is taller — fit to height.
    float scaled_width = kTargetHeight * src_aspect;
    float x_offset = (kTargetWidth - scaled_width) / 2;
    dst_rect = SkRect::MakeXYWH(x_offset, 0, scaled_width, kTargetHeight);
  }

  canvas.drawImageRect(bitmap.asImage(), dst_rect, sampling_options);
  return scaled_bitmap;
}

}  // namespace screenshot
