/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
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
  if (!base::ReadFileToString(path, &contents))
    return std::optional<std::string>();
  return contents;
}

}  // namespace

NTPBackgroundImagesSource::NTPBackgroundImagesSource(
    NTPBackgroundImagesService* service)
    : service_(service),
      weak_factory_(this) {
}

NTPBackgroundImagesSource::~NTPBackgroundImagesSource() = default;

std::string NTPBackgroundImagesSource::GetSource() {
  return kBackgroundWallpaperHost;
}

void NTPBackgroundImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string path = URLDataSource::URLToRequestPath(url);
  auto* images_data = service_->GetBackgroundImagesData();
  const int index = GetWallpaperIndexFromPath(path);

  if (!images_data || index == -1) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));
    return;
  }

  base::FilePath image_file_path =
      images_data->backgrounds[GetWallpaperIndexFromPath(path)].image_file;

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
  if (!input)
    return;

  scoped_refptr<base::RefCountedMemory> bytes;
  bytes = new base::RefCountedBytes(
       reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(callback).Run(std::move(bytes));
}

std::string NTPBackgroundImagesSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  const auto file_path = base::FilePath::FromUTF8Unsafe(path);
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".jpg"))) {
    return "image/jpeg";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".png"))) {
    return "image/png";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".webp"))) {
    return "image/webp";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".avif"))) {
    return "image/avif";
  } else {
    NOTREACHED_IN_MIGRATION();
    return "image/jpeg";
  }
}

int NTPBackgroundImagesSource::GetWallpaperIndexFromPath(
    const std::string& path) const {
  auto* images_data = service_->GetBackgroundImagesData();
  if (!images_data)
    return -1;

  const int wallpaper_count = images_data->backgrounds.size();
  for (int i = 0; i < wallpaper_count; ++i) {
    const std::string image_name =
        images_data->backgrounds[i].image_file.BaseName().AsUTF8Unsafe();
    if (path.compare(image_name) == 0)
      return i;
  }

  return -1;
}

}  // namespace ntp_background_images
