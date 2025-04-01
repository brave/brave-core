// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/upload_file_helper.h"

#include <memory>
#include <utility>

#include "base/barrier_callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/content/browser/full_screenshotter.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace {
data_decoder::DataDecoder* GetDataDecoder() {
  static base::NoDestructor<data_decoder::DataDecoder> data_decoder;
  return data_decoder.get();
}
}  // namespace

namespace ai_chat {

namespace {
using UploadImageCallback = mojom::AIChatUIHandler::UploadImageCallback;

SkBitmap ScaleBitmap(const SkBitmap& bitmap) {
  constexpr int kTargetWidth = 1024;
  constexpr int kTargetHeight = 768;

  // Don't need to scale if dimensions are already smaller than target
  // dimensions
  if (bitmap.width() <= kTargetWidth && bitmap.height() <= kTargetHeight) {
    return bitmap;
  }

  SkBitmap scaled_bitmap;
  scaled_bitmap.allocN32Pixels(kTargetWidth, kTargetHeight);

  SkCanvas canvas(scaled_bitmap);
  canvas.clear(SK_ColorTRANSPARENT);

  // Use high-quality scaling options
  SkSamplingOptions sampling_options(SkFilterMode::kLinear,
                                     SkMipmapMode::kLinear);

  // Maintain aspect ratio while fitting within target dimensions
  float src_aspect = static_cast<float>(bitmap.width()) / bitmap.height();
  float dst_aspect = static_cast<float>(kTargetWidth) / kTargetHeight;

  SkRect dst_rect;
  if (src_aspect > dst_aspect) {
    // Source is wider - fit to width
    float scaled_height = kTargetWidth / src_aspect;
    float y_offset = (kTargetHeight - scaled_height) / 2;
    dst_rect = SkRect::MakeXYWH(0, y_offset, kTargetWidth, scaled_height);
  } else {
    // Source is taller - fit to height
    float scaled_width = kTargetHeight * src_aspect;
    float x_offset = (kTargetWidth - scaled_width) / 2;
    dst_rect = SkRect::MakeXYWH(x_offset, 0, scaled_width, kTargetHeight);
  }

  // Draw scaled bitmap with high-quality sampling
  canvas.drawImageRect(bitmap.asImage(), dst_rect, sampling_options);

  return scaled_bitmap;
}

// base::ReadFileToBytes doesn't handle content uri so we need to read from
// base::File which covers content uri.
std::optional<std::vector<uint8_t>> ReadFileToBytes(
    const base::FilePath& path) {
  std::optional<int64_t> bytes_to_read = base::GetFileSize(path);
  if (!bytes_to_read) {
    return std::nullopt;
  }
  std::vector<uint8_t> bytes(*bytes_to_read);
  base::File file(path,
                  base::File::Flags::FLAG_OPEN | base::File::Flags::FLAG_READ);
  if (file.IsValid()) {
    std::optional<size_t> bytes_read = file.Read(0, bytes);
    if (bytes_read) {
      return bytes;
    }
  }
  return std::nullopt;
}

void OnImageDecoded(
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback,
    const SkBitmap& decoded_bitmap) {
  if (decoded_bitmap.drawsNothing()) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  auto encode_image = base::BindOnce(
      [](const SkBitmap& decoded_bitmap) {
        return gfx::PNGCodec::EncodeBGRASkBitmap(ScaleBitmap(decoded_bitmap),
                                                 false);
      },
      decoded_bitmap);
  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(callback));
}

void ProcessImageData(
    std::vector<uint8_t> image_data,
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback) {
  data_decoder::DecodeImage(
      GetDataDecoder(), std::move(image_data),
      data_decoder::mojom::ImageCodec::kDefault, true,
      data_decoder::kDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(&OnImageDecoded, std::move(callback)));
}

}  // namespace

UploadFileHelper::UploadFileHelper(content::WebContents* owner_web_contents,
                                   content::WebContents* web_contents,
                                   Profile* profile)
    : owner_web_contents_(owner_web_contents),
      web_contents_(web_contents),
      profile_(profile) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

UploadFileHelper::~UploadFileHelper() = default;

void UploadFileHelper::UploadImage(std::unique_ptr<ui::SelectFilePolicy> policy,
                                   mojom::UploadImageOptionsPtr options,
                                   UploadImageCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  upload_image_callback_ = std::move(callback);

  if (options && options->use_screenshots) {
    full_screenshotter_ = std::make_unique<FullScreenshotter>();
    full_screenshotter_->CaptureScreenshots(
        web_contents_, base::BindOnce(&UploadFileHelper::OnScreenshotsCaptured,
                                      weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  select_file_dialog_ = ui::SelectFileDialog::Create(this, std::move(policy));
  ui::SelectFileDialog::FileTypeInfo info;
  info.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  info.extensions = {{FILE_PATH_LITERAL("png"), FILE_PATH_LITERAL("jpeg"),
                      FILE_PATH_LITERAL("jpg"), FILE_PATH_LITERAL("webp")}};
#if BUILDFLAG(IS_ANDROID)
  // Set the list of acceptable MIME types for the file picker; this will apply
  // to any subsequent SelectFile() calls.
  select_file_dialog_->SetAcceptTypes(
      {u"image/png", u"image/jpeg", u"image/jpg", u"image/webp"});
  if (options && options->use_media_capture) {
    select_file_dialog_->SetUseMediaCapture(use_media_capture);
  }
#endif
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE, std::u16string(),
      profile_->last_selected_directory(), &info, 0,
      base::FilePath::StringType(),
      owner_web_contents_->GetTopLevelNativeWindow(), nullptr);
}

void UploadFileHelper::FileSelected(const ui::SelectedFileInfo& file,
                                    int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  profile_->set_last_selected_directory(file.path().DirName());
  auto read_image = base::BindOnce(
      [](const ui::SelectedFileInfo& info)
          -> std::tuple<std::optional<std::vector<uint8_t>>, std::string> {
        return std::make_tuple(
            ai_chat::ReadFileToBytes(info.path()),
            base::FilePath(info.display_name).AsUTF8Unsafe());
      },
      file);
  auto on_image_read = base::BindOnce(&UploadFileHelper::OnImageRead,
                                      weak_ptr_factory_.GetWeakPtr());

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(read_image),
                                               std::move(on_image_read));
}

void UploadFileHelper::MultiFilesSelected(
    const std::vector<ui::SelectedFileInfo>& files) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (files.empty()) {
    std::move(upload_image_callback_).Run(std::nullopt);
    return;
  }

  // Create a barrier callback that will be called when all files are processed
  auto barrier_callback = base::BarrierCallback<
      std::tuple<std::optional<std::vector<uint8_t>>, std::string>>(
      files.size(),
      base::BindOnce(
          [](UploadImageCallback callback,
             std::vector<std::tuple<std::optional<std::vector<uint8_t>>,
                                    std::string>> results) {
            std::vector<mojom::UploadedImagePtr> uploaded_images;
            for (const auto& [image_data, filename] : results) {
              if (image_data) {
                uploaded_images.push_back(mojom::UploadedImage::New(
                    std::move(filename), image_data->size(), *image_data));
              }
            }
            std::move(callback).Run(
                uploaded_images.empty()
                    ? std::nullopt
                    : std::make_optional(std::move(uploaded_images)));
          },
          std::move(upload_image_callback_)));

  // Process each file
  for (const auto& file : files) {
    auto read_image = base::BindOnce(
        [](const ui::SelectedFileInfo& info)
            -> std::tuple<std::optional<std::vector<uint8_t>>, std::string> {
          return std::make_tuple(
              ai_chat::ReadFileToBytes(info.path()),
              base::FilePath(info.display_name).AsUTF8Unsafe());
        },
        file);

    auto on_image_read = base::BindOnce(
        [](base::OnceCallback<void(
               std::tuple<std::optional<std::vector<uint8_t>>, std::string>)>
               callback,
           std::tuple<std::optional<std::vector<uint8_t>>, std::string>
               result) {
          auto image_data = std::get<0>(result);
          if (!image_data) {
            std::move(callback).Run(
                std::make_tuple(std::nullopt, std::get<1>(result)));
            return;
          }
          ProcessImageData(
              std::move(*image_data),
              base::BindOnce(
                  [](base::OnceCallback<void(
                         std::tuple<std::optional<std::vector<uint8_t>>,
                                    std::string>)> callback,
                     std::string filename,
                     std::optional<std::vector<uint8_t>> output) {
                    std::move(callback).Run(std::make_tuple(
                        std::move(output), std::move(filename)));
                  },
                  std::move(callback), std::get<1>(result)));
        },
        barrier_callback);

    base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                                 std::move(read_image),
                                                 std::move(on_image_read));
  }
}

void UploadFileHelper::FileSelectionCanceled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (upload_image_callback_) {
    std::move(upload_image_callback_).Run(std::nullopt);
  }
}

void UploadFileHelper::OnImageRead(
    std::tuple<std::optional<std::vector<uint8_t>>, std::string> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto image_data = std::get<0>(result);
  if (!image_data) {
    std::move(upload_image_callback_).Run(std::nullopt);
    return;
  }
  ProcessImageData(
      std::move(*image_data),
      base::BindOnce(&UploadFileHelper::OnImageEncoded,
                     weak_ptr_factory_.GetWeakPtr(), std::get<1>(result)));
}

void UploadFileHelper::OnImageEncoded(
    std::string filename,
    std::optional<std::vector<uint8_t>> output) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!output) {
    std::move(upload_image_callback_).Run(std::nullopt);
    return;
  }
  std::vector<mojom::UploadedImagePtr> images;
  images.push_back(
      mojom::UploadedImage::New(std::move(filename), output->size(), *output));
  std::move(upload_image_callback_).Run(std::make_optional(std::move(images)));
}

void UploadFileHelper::OnScreenshotsCaptured(
    base::expected<std::vector<std::vector<uint8_t>>, std::string> result) {
  if (result.has_value()) {
    std::vector<mojom::UploadedImagePtr> images;
    size_t screenshot_index = 0;
    for (auto& screenshot : result.value()) {
      size_t screenshot_size = screenshot.size();
      images.push_back(mojom::UploadedImage::New(
          base::StringPrintf("fullscreenshot_%i.png", screenshot_index++),
          screenshot_size, std::move(screenshot)));
    }
    std::move(upload_image_callback_).Run(std::move(images));
  } else {
    VLOG(1) << result.error();
    std::move(upload_image_callback_).Run(std::nullopt);
  }
}

}  // namespace ai_chat
