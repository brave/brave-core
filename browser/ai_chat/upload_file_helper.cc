// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/upload_file_helper.h"

#include <utility>

#include "base/barrier_callback.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "printing/printing_utils.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/cpp/decode_image.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace ai_chat {

namespace {

using UploadFileCallback = mojom::AIChatUIHandler::UploadFileCallback;

std::optional<mojom::UploadedFileType> DetermineFileType(
    const base::FilePath& file_path,
    const std::vector<uint8_t>& file_data) {
  // If extension is .pdf, validate it looks like a PDF
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".pdf"))) {
    if (printing::LooksLikePdf(file_data)) {
      return mojom::UploadedFileType::kPdf;
    }
    return std::nullopt;
  }

  // If extension is an image type, treat as image
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".png")) ||
      file_path.MatchesExtension(FILE_PATH_LITERAL(".jpeg")) ||
      file_path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
      file_path.MatchesExtension(FILE_PATH_LITERAL(".webp"))) {
    return mojom::UploadedFileType::kImage;
  }

  // If no recognized extension, return nullopt
  return std::nullopt;
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

std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath>
ReadSelectedFile(const ui::SelectedFileInfo& info) {
  return std::make_tuple(ai_chat::ReadFileToBytes(info.path()),
                         base::FilePath(info.display_name));
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
        return gfx::PNGCodec::EncodeBGRASkBitmap(
            ScaleDownBitmap(decoded_bitmap), false);
      },
      decoded_bitmap);
  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(encode_image),
                                               std::move(callback));
}

}  // namespace

UploadFileHelper::UploadFileHelper(content::WebContents* web_contents,
                                   Profile* profile)
    : web_contents_(web_contents), profile_(profile) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

UploadFileHelper::~UploadFileHelper() = default;

void UploadFileHelper::UploadFile(std::unique_ptr<ui::SelectFilePolicy> policy,
#if BUILDFLAG(IS_ANDROID)
                                  bool use_media_capture,
#endif
                                  UploadFileCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  upload_file_callback_ = std::move(callback);

  select_file_dialog_ = ui::SelectFileDialog::Create(this, std::move(policy));
  ui::SelectFileDialog::FileTypeInfo info;
  info.allowed_paths = ui::SelectFileDialog::FileTypeInfo::NATIVE_PATH;
  info.extensions = {{FILE_PATH_LITERAL("png"), FILE_PATH_LITERAL("jpeg"),
                      FILE_PATH_LITERAL("jpg"), FILE_PATH_LITERAL("webp"),
                      FILE_PATH_LITERAL("pdf")}};
#if BUILDFLAG(IS_ANDROID)
  // Set the list of acceptable MIME types for the file picker; this will apply
  // to any subsequent SelectFile() calls.
  select_file_dialog_->SetAcceptTypes(
      {u"image/png", u"image/jpeg", u"image/webp", u"application/pdf"});
  select_file_dialog_->SetUseMediaCapture(use_media_capture);
#endif
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_MULTI_FILE, std::u16string(),
      profile_->last_selected_directory(), &info, 0,
      base::FilePath::StringType(), web_contents_->GetTopLevelNativeWindow(),
      nullptr);
}

void UploadFileHelper::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void UploadFileHelper::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
void UploadFileHelper::ProcessImageData(
    data_decoder::DataDecoder* data_decoder,
    const std::vector<uint8_t>& image_data,
    base::OnceCallback<void(std::optional<std::vector<uint8_t>>)> callback) {
  data_decoder::DecodeImage(
      data_decoder, image_data, data_decoder::mojom::ImageCodec::kDefault, true,
      data_decoder::kDefaultMaxSizeInBytes, gfx::Size(),
      base::BindOnce(&OnImageDecoded, std::move(callback)));
}

void UploadFileHelper::FileSelected(const ui::SelectedFileInfo& file,
                                    int index) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (auto& observer : observers_) {
    observer.OnFilesSelected();
  }
  profile_->set_last_selected_directory(file.path().DirName());
  auto read_file = base::BindOnce(&ReadSelectedFile, file);
  auto on_file_read = base::BindOnce(&UploadFileHelper::OnFileRead,
                                     weak_ptr_factory_.GetWeakPtr());

  base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                               std::move(read_file),
                                               std::move(on_file_read));
}

void UploadFileHelper::MultiFilesSelected(
    const std::vector<ui::SelectedFileInfo>& files) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (files.empty()) {
    std::move(upload_file_callback_).Run(std::nullopt);
    return;
  }
  for (auto& observer : observers_) {
    observer.OnFilesSelected();
  }

  // Create a barrier callback that will be called when all files are processed
  auto barrier_callback = base::BarrierCallback<
      std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath,
                 std::optional<mojom::UploadedFileType>>>(
      files.size(),
      base::BindOnce(
          [](UploadFileCallback callback,
             std::vector<
                 std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath,
                            std::optional<mojom::UploadedFileType>>> results) {
            std::vector<mojom::UploadedFilePtr> uploaded_files;
            for (const auto& [file_data, filepath, file_type_opt] : results) {
              if (file_data && file_type_opt) {
                uploaded_files.push_back(mojom::UploadedFile::New(
                    filepath.AsUTF8Unsafe(), file_data->size(), *file_data,
                    *file_type_opt));
              }
            }
            std::move(callback).Run(
                uploaded_files.empty()
                    ? std::nullopt
                    : std::make_optional(std::move(uploaded_files)));
          },
          std::move(upload_file_callback_)));

  // Process each file
  for (const auto& file : files) {
    auto read_file = base::BindOnce(&ReadSelectedFile, file);

    auto on_file_read = base::BindOnce(
        [](data_decoder::DataDecoder* data_decoder,
           base::OnceCallback<void(
               std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath,
                          std::optional<mojom::UploadedFileType>>)> callback,
           std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath>
               result) {
          auto file_data = std::get<0>(result);
          auto filepath = std::get<1>(result);

          if (!file_data) {
            std::move(callback).Run(std::make_tuple(
                std::nullopt, std::move(filepath), std::nullopt));
            return;
          }

          // Determine file type based on extension and validate PDF content
          auto file_type_opt = DetermineFileType(filepath, *file_data);

          if (file_type_opt &&
              *file_type_opt == mojom::UploadedFileType::kPdf) {
            // For PDFs, just return the raw data without processing
            std::move(callback).Run(std::make_tuple(
                std::move(file_data), std::move(filepath), file_type_opt));
          } else if (file_type_opt &&
                     *file_type_opt == mojom::UploadedFileType::kImage) {
            // For images, process them as before
            UploadFileHelper::ProcessImageData(
                data_decoder, *file_data,
                base::BindOnce(
                    [](base::OnceCallback<void(
                           std::tuple<std::optional<std::vector<uint8_t>>,
                                      base::FilePath,
                                      std::optional<mojom::UploadedFileType>>)>
                           callback,
                       base::FilePath filepath,
                       std::optional<mojom::UploadedFileType> file_type_opt,
                       std::optional<std::vector<uint8_t>> output) {
                      std::move(callback).Run(
                          std::make_tuple(std::move(output),
                                          std::move(filepath), file_type_opt));
                    },
                    std::move(callback), std::move(filepath), file_type_opt));
          } else {
            // Fail if we cannot handle this file type
            std::move(callback).Run(std::make_tuple(
                std::nullopt, std::move(filepath), std::nullopt));
          }
        },
        &data_decoder_, barrier_callback);

    base::ThreadPool::PostTaskAndReplyWithResult(FROM_HERE, {base::MayBlock()},
                                                 std::move(read_file),
                                                 std::move(on_file_read));
  }
}

void UploadFileHelper::FileSelectionCanceled() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (upload_file_callback_) {
    std::move(upload_file_callback_).Run(std::nullopt);
  }
}

void UploadFileHelper::OnFileRead(
    std::tuple<std::optional<std::vector<uint8_t>>, base::FilePath> result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto file_data = std::get<0>(result);
  if (!file_data) {
    std::move(upload_file_callback_).Run(std::nullopt);
    return;
  }

  // Determine file type based on extension and validate PDF content
  auto file_type_opt = DetermineFileType(std::get<1>(result), *file_data);

  if (file_type_opt && *file_type_opt == mojom::UploadedFileType::kPdf) {
    // For PDFs, just return the raw data without processing
    std::vector<mojom::UploadedFilePtr> files;
    files.push_back(mojom::UploadedFile::New(std::get<1>(result).AsUTF8Unsafe(),
                                             file_data->size(), *file_data,
                                             mojom::UploadedFileType::kPdf));
    std::move(upload_file_callback_).Run(std::make_optional(std::move(files)));
  } else if (file_type_opt &&
             *file_type_opt == mojom::UploadedFileType::kImage) {
    // For images, process them as before
    UploadFileHelper::ProcessImageData(
        &data_decoder_, *file_data,
        base::BindOnce(&UploadFileHelper::OnImageEncoded,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::get<1>(result).AsUTF8Unsafe()));
  } else {
    // Fail if we cannot handle this file type
    std::move(upload_file_callback_).Run(std::nullopt);
  }
}

void UploadFileHelper::OnImageEncoded(
    std::string filename,
    std::optional<std::vector<uint8_t>> output) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!output) {
    std::move(upload_file_callback_).Run(std::nullopt);
    return;
  }
  std::vector<mojom::UploadedFilePtr> images;
  images.push_back(mojom::UploadedFile::New(std::move(filename), output->size(),
                                            *output,
                                            mojom::UploadedFileType::kImage));
  std::move(upload_file_callback_).Run(std::make_optional(std::move(images)));
}

}  // namespace ai_chat
