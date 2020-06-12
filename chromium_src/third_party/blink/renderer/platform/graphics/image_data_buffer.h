/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_

#define BRAVE_IMAGE_DATA_BUFFER_H \
  sk_sp<SkImage> RetainedImage() { return retained_image_; }

#include "../../../../../../../third_party/blink/renderer/platform/graphics/image_data_buffer.h"

#undef BRAVE_IMAGE_DATA_BUFFER_H

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_IMAGE_DATA_BUFFER_H_
