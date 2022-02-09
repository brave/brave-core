// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_background_images_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/image/image.h"

namespace ntp_background_images {

namespace {

absl::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return absl::optional<std::string>();
  return contents;
}

}  // namespace

NTPCustomImagesSource::NTPCustomImagesSource(
    NTPCustomBackgroundImagesService* service,
    std::unique_ptr<image_fetcher::ImageDecoder> image_decoder)
    : service_(service),
      image_decoder_(std::move(image_decoder)),
      weak_factory_(this) {
  DCHECK(service_);
}

NTPCustomImagesSource::~NTPCustomImagesSource() = default;

std::string NTPCustomImagesSource::GetSource() {
  return kCustomWallpaperHost;
}

void NTPCustomImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GetImageFile(service_->GetImageFilePath() , std::move(callback));
}

std::string NTPCustomImagesSource::GetMimeType(const std::string& path) {
  return "image/jpeg";
}

void NTPCustomImagesSource::GetImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPCustomImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPCustomImagesSource::OnGotImageFile(
    GotDataCallback callback,
    absl::optional<std::string> input) {
  if (!input) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));
    return;
  }

  // Send image body to image decoder in isolated process.
  image_decoder_->DecodeImage(
      *input, gfx::Size() /* No particular size desired. */,
      base::BindOnce(&NTPCustomImagesSource::OnImageDecoded,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPCustomImagesSource::OnImageDecoded(
    content::URLDataSource::GotDataCallback callback,
    const gfx::Image& image) {
  // Re-encode vetted image as PNG and send to requester.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](const SkBitmap& bitmap) {
            auto encoded = base::MakeRefCounted<base::RefCountedBytes>();
            return gfx::PNGCodec::EncodeBGRASkBitmap(
                       bitmap, /*discard_transparency=*/false, &encoded->data())
                       ? encoded
                       : base::MakeRefCounted<base::RefCountedBytes>();
          },
          image.AsBitmap()),
      std::move(callback));
}

}  // namespace ntp_background_images
