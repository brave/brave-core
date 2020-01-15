/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/ntp_sponsored_image_source.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/components/ntp_sponsored_images/url_constants.h"
#include "content/public/browser/browser_thread.h"

namespace {
base::Optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return base::Optional<std::string>();
  return contents;
}

}  // namespace

NTPSponsoredImageSource::NTPSponsoredImageSource(
    const NTPSponsoredImagesInternalData& internal_images_data)
    : images_data_(internal_images_data),
      weak_factory_(this) {
}

NTPSponsoredImageSource::~NTPSponsoredImageSource() = default;

std::string NTPSponsoredImageSource::GetSource() {
  return kBrandedWallpaperHost;
}

void NTPSponsoredImageSource::StartDataRequest(
    const std::string& path,
    const content::WebContents::Getter& wc_getter,
    const GotDataCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!IsValidPath(path)) {
    scoped_refptr<base::RefCountedMemory> bytes;
    std::move(callback).Run(std::move(bytes));
    return;
  }

  base::FilePath image_file_path;
  if (IsLogoPath(path)) {
    image_file_path = images_data_.logo_image_file;
  } else {
    DCHECK(IsWallpaperPath(path));
    image_file_path =
        images_data_.wallpaper_image_files[GetWallpaperIndexFromPath(path)];
  }

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPSponsoredImageSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(),
                     callback));
}

void NTPSponsoredImageSource::OnGotImageFile(
    const GotDataCallback& callback,
    base::Optional<std::string> input) {
  if (!input)
    return;

  scoped_refptr<base::RefCountedMemory> bytes;
  bytes = new base::RefCountedBytes(
       reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(callback).Run(std::move(bytes));
}

std::string NTPSponsoredImageSource::GetMimeType(const std::string& path) {
  if (IsLogoPath(path))
    return "image/png";
  return "image/jpg";
}

bool NTPSponsoredImageSource::IsValidPath(const std::string& path) const {
  if (IsLogoPath(path))
    return true;

  if (IsWallpaperPath(path))
    return true;

  return false;
}

bool NTPSponsoredImageSource::IsWallpaperPath(const std::string& path) const {
  return GetWallpaperIndexFromPath(path) != -1;
}

bool NTPSponsoredImageSource::IsLogoPath(const std::string& path) const {
  return path.compare(kLogoPath) == 0;
}

int NTPSponsoredImageSource::GetWallpaperIndexFromPath(
    const std::string& path) const {
  const int wallpaper_count = images_data_.wallpaper_image_files.size();
  for (int i = 0; i < wallpaper_count; ++i) {
    const std::string generated_path =
        base::StringPrintf("%s%d.jpg", kWallpaperPathPrefix, i);
    if (path.compare(generated_path) == 0)
      return i;
  }

  return -1;
}
