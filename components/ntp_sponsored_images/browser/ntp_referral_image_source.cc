/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_sponsored_images/browser/ntp_referral_image_source.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_service.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_referral_images_data.h"
#include "brave/components/ntp_sponsored_images/browser/url_constants.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace ntp_sponsored_images {

namespace {

base::Optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return base::Optional<std::string>();
  return contents;
}

}  // namespace

NTPReferralImageSource::NTPReferralImageSource(
    NTPReferralImagesService* service)
    : service_(service),
      weak_factory_(this) {
}

NTPReferralImageSource::~NTPReferralImageSource() = default;

std::string NTPReferralImageSource::GetSource() {
  return kReferralWallpaperHost;
}

void NTPReferralImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string path = URLDataSource::URLToRequestPath(url);
  if (!IsValidPath(path)) {
    scoped_refptr<base::RefCountedMemory> bytes;
    std::move(callback).Run(std::move(bytes));
    return;
  }

  auto* images_data = service_->GetReferralImagesData();
  if (!images_data) {
    base::PostTask(FROM_HERE,
        base::BindOnce(std::move(callback),
                       scoped_refptr<base::RefCountedMemory>()));
    return;
  }

  base::FilePath image_file_path;
  if (IsLogoPath(path)) {
    image_file_path = images_data->logo_image_file;
  } else if (IsIconPath(path)) {
    image_file_path =
        images_data->top_sites[GetIconFileIndexFromPath(path)].icon_image_file;
  } else {
    DCHECK(IsWallpaperPath(path));
    image_file_path =
        images_data->wallpaper_image_files[GetWallpaperIndexFromPath(path)];
  }

  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPReferralImageSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void NTPReferralImageSource::OnGotImageFile(
    GotDataCallback callback,
    base::Optional<std::string> input) {
  if (!input)
    return;

  scoped_refptr<base::RefCountedMemory> bytes;
  bytes = new base::RefCountedBytes(
       reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(callback).Run(std::move(bytes));
}

std::string NTPReferralImageSource::GetMimeType(const std::string& path) {
  if (IsLogoPath(path))
    return "image/png";
  return "image/jpg";
}

bool NTPReferralImageSource::AllowCaching() {
  return false;
}

bool NTPReferralImageSource::IsValidPath(const std::string& path) const {
  if (IsLogoPath(path))
    return true;

  if (IsWallpaperPath(path))
    return true;

  if (IsIconPath(path))
    return true;

  return false;
}

int NTPReferralImageSource::GetIconFileIndexFromPath(
    const std::string& path) const {
  auto* images_data = service_->GetReferralImagesData();
  if (!images_data)
    return -1;

  const int icon_count = images_data->top_sites.size();
  for (int i = 0; i < icon_count; ++i) {
    GURL url(images_data->top_sites[i].icon_image_url());
    if (path.compare(URLDataSource::URLToRequestPath(url)) == 0)
      return i;
  }

  return -1;
}

bool NTPReferralImageSource::IsIconPath(const std::string& path) const {
  return GetIconFileIndexFromPath(path) != -1;
}

bool NTPReferralImageSource::IsWallpaperPath(const std::string& path) const {
  return GetWallpaperIndexFromPath(path) != -1;
}

bool NTPReferralImageSource::IsLogoPath(const std::string& path) const {
  return path.compare(kLogoPath) == 0;
}

int NTPReferralImageSource::GetWallpaperIndexFromPath(
    const std::string& path) const {
  auto* images_data = service_->GetReferralImagesData();
  if (!images_data)
    return -1;

  const int wallpaper_count = images_data->wallpaper_image_files.size();
  for (int i = 0; i < wallpaper_count; ++i) {
    const std::string generated_path =
        base::StringPrintf("%s%d.jpg", kWallpaperPathPrefix, i);
    if (path.compare(generated_path) == 0)
      return i;
  }

  return -1;
}

}  // namespace ntp_sponsored_images
