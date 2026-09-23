/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"

#include <optional>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace ntp_background_images {

namespace {

std::optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    return std::nullopt;
  }
  return contents;
}

}  // namespace

NTPBackgroundImagesSource::NTPBackgroundImagesSource(
    NTPBackgroundImagesService* background_images_service)
    : background_images_service_(background_images_service) {}

NTPBackgroundImagesSource::~NTPBackgroundImagesSource() = default;

std::string NTPBackgroundImagesSource::GetSource() {
  return kBackgroundWallpaperHost;
}

void NTPBackgroundImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& /*wc_getter*/,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const NTPBackgroundImagesData* const images_data =
      background_images_service_->GetBackgroundImagesData();
  const std::string path = URLDataSource::URLToRequestPath(url);
  const int index = GetWallpaperIndexFromPath(path);

  if (!images_data || index == -1) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));
    return;
  }

  base::FilePath image_file_path =
      images_data->backgrounds[GetWallpaperIndexFromPath(path)].file_path;

  GetImageFile(image_file_path, std::move(callback));
}

void NTPBackgroundImagesSource::GetImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPBackgroundImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPBackgroundImagesSource::OnGotImageFile(
    GotDataCallback callback,
    std::optional<std::string> input) {
  if (!input) {
    return;
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string NTPBackgroundImagesSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  const base::FilePath file_path = base::FilePath::FromUTF8Unsafe(path);
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".jpg"))) {
    return "image/jpeg";
  }
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".png"))) {
    return "image/png";
  }
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".webp"))) {
    return "image/webp";
  }
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".avif"))) {
    return "image/avif";
  }
  return "";
}

int NTPBackgroundImagesSource::GetWallpaperIndexFromPath(
    const std::string& path) const {
  const NTPBackgroundImagesData* const images_data =
      background_images_service_->GetBackgroundImagesData();
  if (!images_data) {
    return -1;
  }

  for (size_t i = 0; i < images_data->backgrounds.size(); ++i) {
    const std::string image_name =
        images_data->backgrounds[i].file_path.BaseName().AsUTF8Unsafe();
    if (path == image_name) {
      return static_cast<int>(i);
    }
  }

  return -1;
}

}  // namespace ntp_background_images
