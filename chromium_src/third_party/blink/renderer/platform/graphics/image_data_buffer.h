/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_

// Providing multable access to ImageDataBuffer::pixmap_ to allow us to call
// `PerturbPixels` on it.
#define EncodeImage            \
  EncodeImage_Unused();        \
  SkPixmap pixmap_multable() { \
    return pixmap_;            \
  }                            \
  bool EncodeImage
#include <third_party/blink/renderer/platform/graphics/image_data_buffer.h>  // IWYU pragma: export
#undef EncodeImage

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_
