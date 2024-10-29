/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ntp_background/custom_background_file_manager.h"

#include <utility>

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/browser/ntp_background/constants.h"
#include "chrome/browser/image_fetcher/image_decoder_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"

namespace {

data_decoder::DataDecoder* GetDataDecoder() {
  static base::NoDestructor<data_decoder::DataDecoder> data_decoder;
  return data_decoder.get();
}

}  // namespace

CustomBackgroundFileManager::CustomBackgroundFileManager(Profile* profile)
    : profile_(profile) {}

CustomBackgroundFileManager::~CustomBackgroundFileManager() = default;

void CustomBackgroundFileManager::SaveImage(
    const base::FilePath& source_file_path,
    SaveFileCallback callback) {
  auto copy_sanitized_image = base::BindOnce(
      [](base::WeakPtr<CustomBackgroundFileManager> self,
         const base::FilePath& source_file_path, SaveFileCallback callback,
         bool dir_exists) {
        if (!self)
          return;

        if (!dir_exists) {
          DVLOG(2) << "Failed to create custom background directory";
          std::move(callback).Run(base::FilePath());
          return;
        }

        const auto target_path = self->GetCustomBackgroundDirectory().Append(
            source_file_path.BaseName());
        self->ReadImage(
            source_file_path,
            base::BindOnce(&CustomBackgroundFileManager::SanitizeAndSaveImage,
                           self, std::move(callback), target_path));
      },
      weak_factory_.GetWeakPtr(), source_file_path, std::move(callback));

  MakeSureDirExists(std::move(copy_sanitized_image));
}

void CustomBackgroundFileManager::MoveImage(
    const base::FilePath& source_file_path,
    base::OnceCallback<void(bool /*result*/)> callback) {
  auto move_file = base::BindOnce(
      [](const base::FilePath& source_file, const base::FilePath& target_path) {
        base::File::Info info;
        if (!base::GetFileInfo(source_file, &info) || info.is_directory) {
          DVLOG(2) << "Failed to move file: source image file is invalid";
          return false;
        }

        if (!base::Move(source_file, target_path)) {
          DVLOG(2) << "Failed to move file from " << source_file << " to "
                   << target_path;
          return false;
        }

        return true;
      },
      source_file_path,
      GetCustomBackgroundDirectory().Append(source_file_path.BaseName()));

  auto on_check_dir = base::BindOnce(
      [](base::OnceCallback<bool()> move,
         base::OnceCallback<void(bool /*result*/)> callback, bool dir_exists) {
        if (!dir_exists) {
          DVLOG(2) << "Failed to create custom background directory";
          std::move(callback).Run(false);
          return;
        }

        base::ThreadPool::PostTaskAndReplyWithResult(
            FROM_HERE, {base::MayBlock()}, std::move(move),
            std::move(callback));
      },
      std::move(move_file), std::move(callback));

  MakeSureDirExists(std::move(on_check_dir));
}

void CustomBackgroundFileManager::RemoveImage(
    const base::FilePath& file_path,
    base::OnceCallback<void(bool /*result*/)> callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(base::DeleteFile, file_path), std::move(callback));
}

base::FilePath CustomBackgroundFileManager::GetCustomBackgroundDirectory()
    const {
  return profile_->GetPath().AppendASCII(
      ntp_background_images::kSanitizedImageDirName);
}

void CustomBackgroundFileManager::MakeSureDirExists(
    base::OnceCallback<void(bool /* dir_exists*/)> on_dir_check) {
  auto check_dir = base::BindOnce(
      [](const base::FilePath& dir_path) -> bool {
        return base::DirectoryExists(dir_path) ||
               base::CreateDirectory(dir_path);
      },
      GetCustomBackgroundDirectory());

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(check_dir),
                                               std::move(on_dir_check));
}

void CustomBackgroundFileManager::ReadImage(
    const base::FilePath& path,
    base::OnceCallback<void(const std::string&)> on_got_image) {
  DCHECK(!path.empty());

  auto read_image = base::BindOnce(
      [](const base::FilePath& image_file_path) -> std::string {
        std::string contents;
        base::ReadFileToString(image_file_path, &contents);
        return contents;
      },
      path);

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(read_image),
                                               std::move(on_got_image));
}

void CustomBackgroundFileManager::SanitizeAndSaveImage(
    SaveFileCallback callback,
    const base::FilePath& target_file_path,
    const std::string& input) {
  if (input.empty()) {
    DVLOG(2) << __func__ << "given |input| is empty";
    std::move(callback).Run(base::FilePath());
    return;
  }

  auto re_encode_decoded_image = base::BindOnce(
      &CustomBackgroundFileManager::SaveImageAsPNG, weak_factory_.GetWeakPtr(),
      std::move(callback), target_file_path);

  DecodeImageInIsolatedProcess(input, std::move(re_encode_decoded_image));
}

void CustomBackgroundFileManager::DecodeImageInIsolatedProcess(
    const std::string& input,
    base::OnceCallback<void(const gfx::Image&)> on_decode) {
  DCHECK(!input.empty());

  if (!image_decoder_)
    image_decoder_ = std::make_unique<ImageDecoderImpl>();

  image_decoder_->DecodeImage(input,
                              gfx::Size() /* No particular size desired. */,
                              GetDataDecoder(), std::move(on_decode));
}

void CustomBackgroundFileManager::SaveImageAsPNG(
    SaveFileCallback callback,
    const base::FilePath& target_path,
    const gfx::Image& image) {
  if (image.IsEmpty()) {
    DVLOG(2) << __func__ << "Given |image| is empty";
    return std::move(callback).Run(base::FilePath());
  }
  auto encode_and_save = base::BindOnce(
      [](const SkBitmap& bitmap, const base::FilePath& target_path) {
        std::optional<std::vector<uint8_t>> encoded =
            gfx::PNGCodec::EncodeBGRASkBitmap(bitmap,
                                              /*discard_transparency=*/false);
        if (!encoded) {
          DVLOG(2) << "Failed to encode image as PNG";
          return base::FilePath();
        }

        base::FilePath modified_path = target_path;
        for (int i = 1; base::PathExists(modified_path); ++i) {
          modified_path = target_path.InsertBeforeExtensionASCII(
              base::StringPrintf("-%d", i));
        }

        if (!base::WriteFile(modified_path, *encoded)) {
          DVLOG(2) << "Failed to write image to file " << modified_path;
          return base::FilePath();
        }

        return modified_path;
      },
      image.AsBitmap(), target_path);

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_and_save),
                                               std::move(callback));
}
