/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/test_utils.h"

#include "base/rand_util.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "crypto/random.h"

namespace ai_chat {

std::vector<mojom::UploadedImagePtr> CreateSampleUploadedImages(size_t number) {
  std::vector<mojom::UploadedImagePtr> uploaded_images;
  for (size_t i = 0; i < number; ++i) {
    std::vector<uint8_t> image_data(base::RandGenerator(64));
    crypto::RandBytes(image_data);
    uploaded_images.emplace_back(mojom::UploadedImage::New(
        "filename" + base::NumberToString(i), sizeof(image_data), image_data));
  }
  return uploaded_images;
}
std::vector<mojom::UploadedImagePtr> CloneUpdatedImages(
    const std::vector<mojom::UploadedImagePtr>& input) {
  std::vector<mojom::UploadedImagePtr> output;
  for (const auto& image : input) {
    output.emplace_back(image.Clone());
  }
  return output;
}

}  // namespace ai_chat
