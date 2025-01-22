// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/upload_file_helper.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "third_party/skia/include/core/SkBitmap.h"
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
}  // namespace

UploadFileHelper::UploadFileHelper(content::WebContents* web_contents,
                                   Profile* profile)
    : web_contents_(web_contents), profile_(profile) {}

UploadFileHelper::~UploadFileHelper() = default;

void UploadFileHelper::UploadImage(std::unique_ptr<ui::SelectFilePolicy> policy,
                                   UploadImageCallback callback) {
  select_file_dialog_ = ui::SelectFileDialog::Create(this, std::move(policy));
  ui::SelectFileDialog::FileTypeInfo info;
  info.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  info.extensions = {{"png", "jpeg", "jpg", "webp"}};
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, std::u16string(),
      profile_->last_selected_directory(), &info, 0,
      base::FilePath::StringType(), web_contents_->GetTopLevelNativeWindow(),
      nullptr);
  upload_image_callback_ = std::move(callback);
}

void UploadFileHelper::FileSelected(const ui::SelectedFileInfo& file,
                                    int index) {
  profile_->set_last_selected_directory(file.path().DirName());
  auto read_image = base::BindOnce(
      [](const ui::SelectedFileInfo& info)
          -> std::tuple<std::optional<std::vector<uint8_t>>,
                        std::optional<std::string>, std::optional<int64_t>> {
        return std::make_tuple(base::ReadFileToBytes(info.path()),
                               info.display_name,
                               base::GetFileSize(info.path()));
      },
      file);
  auto on_image_read = base::BindOnce(&UploadFileHelper::OnImageRead,
                                      weak_ptr_factory_.GetWeakPtr());

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(read_image),
                                               std::move(on_image_read));
}

void UploadFileHelper::RemoveUploadedImage(uint32_t index) {
  if (index >= uploaded_images_.size()) {
    return;
  }
  uploaded_images_.erase(uploaded_images_.begin() + index);
}

void UploadFileHelper::FileSelectionCanceled() {
  if (upload_image_callback_) {
    std::move(upload_image_callback_)
        .Run(std::nullopt, std::nullopt, std::nullopt);
  }
}

void UploadFileHelper::OnImageRead(
    std::tuple<std::optional<std::vector<uint8_t>>,
               std::optional<std::string>,
               std::optional<int64_t>> result) {
  auto image_data = std::get<0>(result);
  if (!image_data) {
    std::move(upload_image_callback_)
        .Run(std::get<0>(result), std::get<1>(result), std::get<2>(result));
  }
  auto on_image_sanitized = base::BindOnce(
      &UploadFileHelper::OnImageSanitized, weak_ptr_factory_.GetWeakPtr(),
      std::get<1>(result), std::get<2>(result));
  data_decoder::DecodeImage(
      GetDataDecoder(), *image_data, data_decoder::mojom::ImageCodec::kDefault,
      true, data_decoder::kDefaultMaxSizeInBytes, gfx::Size(1024, 768),
      std::move(on_image_sanitized));
}

void UploadFileHelper::OnImageSanitized(std::optional<std::string> filename,
                                        std::optional<int64_t> filesize,
                                        const SkBitmap& decoded_bitmap) {
  auto encode_image = base::BindOnce(
      [](const SkBitmap& decoded_bitmap) {
        return gfx::PNGCodec::EncodeBGRASkBitmap(decoded_bitmap, false);
      },
      decoded_bitmap);
  auto on_image_encoded =
      base::BindOnce(&UploadFileHelper::OnImageEncoded,
                     weak_ptr_factory_.GetWeakPtr(), filename, filesize);

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(on_image_encoded));
}

void UploadFileHelper::OnImageEncoded(
    std::optional<std::string> filename,
    std::optional<int64_t> filesize,
    std::optional<std::vector<uint8_t>> output) {
  if (output) {
    uploaded_images_.push_back(*output);
  }
  std::move(upload_image_callback_)
      .Run(std::move(output), std::move(filename), std::move(filesize));
}

std::vector<std::vector<uint8_t>>& UploadFileHelper::GetUploadedImages() {
  return uploaded_images_;
}

size_t UploadFileHelper::GetUploadedImagesSize() {
  return uploaded_images_.size();
}

}  // namespace ai_chat
