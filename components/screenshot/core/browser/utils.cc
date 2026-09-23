// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/screenshot/core/browser/utils.h"

#include <algorithm>

#include "base/numerics/safe_conversions.h"
#include "skia/ext/image_operations.h"

namespace screenshot {

SkBitmap ScaleDownBitmap(const SkBitmap& bitmap) {
  constexpr int kTargetWidth = 1024;
  constexpr int kTargetHeight = 768;

  if (bitmap.width() <= kTargetWidth && bitmap.height() <= kTargetHeight) {
    return bitmap;
  }

  const float src_aspect = static_cast<float>(bitmap.width()) / bitmap.height();
  const float dst_aspect = static_cast<float>(kTargetWidth) / kTargetHeight;

  int width;
  int height;
  if (src_aspect > dst_aspect) {
    // Source is wider — fit to width.
    width = kTargetWidth;
    height = std::max(1, base::ClampRound(kTargetWidth / src_aspect));
  } else {
    // Source is taller — fit to height.
    width = std::max(1, base::ClampRound(kTargetHeight * src_aspect));
    height = kTargetHeight;
  }

  // Scale with ImageOperations rather than SkCanvas::drawImageRect() for
  // cross-platform support.
  return skia::ImageOperations::Resize(
      bitmap, skia::ImageOperations::RESIZE_BETTER, width, height);
}

}  // namespace screenshot
