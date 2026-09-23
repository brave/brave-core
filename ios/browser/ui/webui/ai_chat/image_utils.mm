// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/image_utils.h"

#import <Foundation/Foundation.h>

#include "brave/components/screenshot/core/browser/utils.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"

namespace ai_chat {

std::optional<std::vector<uint8_t>> DecodeAndScaleImageData(
    std::vector<uint8_t> image_data) {
  NSData* ns_data = [NSData dataWithBytes:image_data.data()
                                   length:image_data.size()];
  // ImageIO decodes, then the shared util scales, so uploads are sized
  // identically on every platform.
  const std::vector<SkBitmap> bitmaps = skia::ImageDataToSkBitmaps(ns_data);
  if (bitmaps.empty()) {
    return std::nullopt;
  }
  return gfx::PNGCodec::EncodeBGRASkBitmap(
      screenshot::ScaleDownBitmap(bitmaps.front()),
      /*discard_transparency=*/false);
}

}  // namespace ai_chat
