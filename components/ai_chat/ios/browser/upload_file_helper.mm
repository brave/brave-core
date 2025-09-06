// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/ios/browser/upload_file_helper.h"

#include <UIKit/UIKit.h>

#include "base/apple/foundation_util.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "ios/web/public/thread/web_task_traits.h"
#include "ios/web/public/thread/web_thread.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "skia/ext/skia_utils_ios.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "ui/gfx/codec/png_codec.h"

namespace {

constexpr std::uint32_t kMaxImageDimension = 1024;

std::optional<std::vector<uint8_t>> DecodeImageOnBackgroundThread(
    const std::vector<uint8_t>& image_data) {
  NSData* ns_data = [NSData dataWithBytes:image_data.data()
                                   length:image_data.size()];
  if (!ns_data) {
    DLOG(ERROR) << "Failed to create NSData from image vector.";
    return std::optional<std::vector<uint8_t>>();
  }

  UIImage* ui_image = [UIImage imageWithData:ns_data];
  if (!ui_image) {
    DLOG(ERROR) << "Failed to decode image data into UIImage.";
    return std::optional<std::vector<uint8_t>>();
  }

  // Calculate the new size while preserving aspect ratio.
  CGSize original_size = ui_image.size;
  CGSize scaled_size = original_size;

  if (original_size.width > kMaxImageDimension ||
      original_size.height > kMaxImageDimension) {
    CGFloat scale_factor = MIN(kMaxImageDimension / original_size.width,
                               kMaxImageDimension / original_size.height);
    scaled_size = CGSizeMake(original_size.width * scale_factor,
                             original_size.height * scale_factor);

    UIGraphicsBeginImageContextWithOptions(scaled_size, NO, 1.0);
    [ui_image
        drawInRect:CGRectMake(0, 0, scaled_size.width, scaled_size.height)];
    UIImage* scaled_ui_image = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();

    if (scaled_ui_image) {
      ui_image = scaled_ui_image;
    }
  }

  CGImageRef cg_image = [ui_image CGImage];
  if (!cg_image) {
    DLOG(ERROR) << "Failed to get CGImageRef from UIImage.";
    return std::optional<std::vector<uint8_t>>();
  }

  CGSize image_size = ui_image.size;
  SkBitmap bitmap =
      skia::CGImageToSkBitmap(cg_image, image_size, NO /* is_opaque */);
  if (bitmap.empty()) {
    DLOG(ERROR) << "Failed to convert CGImage to SkBitmap.";
    return std::optional<std::vector<uint8_t>>();
  }

  std::vector<uint8_t> pixel_data;
  pixel_data.resize(bitmap.rowBytes() * bitmap.height());

  if (!bitmap.readPixels(bitmap.info(), pixel_data.data(), bitmap.rowBytes(), 0,
                         0)) {
    DLOG(ERROR) << "Failed to read pixels into vector.";
    return std::optional<std::vector<uint8_t>>();
  }

  return std::optional<std::vector<uint8_t>>(std::move(pixel_data));
}

}  // namespace

namespace ai_chat {

void OnImageDecoded(
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback,
    const SkBitmap& decoded_bitmap) {
  if (decoded_bitmap.drawsNothing()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto encode_image = base::BindOnce(
      [](const SkBitmap& decoded_bitmap) {
        return gfx::PNGCodec::EncodeBGRASkBitmap(
            ScaleDownBitmap(decoded_bitmap), false);
      },
      decoded_bitmap);
  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(callback));
}

// static
void UploadFileHelperIOS::ProcessImageFileURL(
    NSURL* url,
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback) {
  base::FilePath file_path = base::apple::NSStringToFilePath([url path]);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&base::ReadFileToBytes, file_path),
      base::BindOnce(
          [](base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> cb,
             std::optional<std::vector<uint8_t>> image_data) {
            if (!image_data) {
              DLOG(ERROR) << "Failed to read file into bytes.";
              std::move(cb).Run(std::nullopt);
              return;
            }

            ProcessImageData(std::move(image_data.value()), std::move(cb));
          },
          std::move(callback)));
}

// static
void UploadFileHelperIOS::ProcessImageData(
    const std::vector<uint8_t>& image_data,
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&DecodeImageOnBackgroundThread, image_data),
      std::move(callback));
}

}  // namespace ai_chat
