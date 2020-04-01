/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_background_images_source.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_data.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "content/public/browser/browser_thread.h"

namespace ntp_background_images {

namespace {

base::Optional<std::string> ReadFileToString(const base::FilePath& path) {
  std::string contents;
  if (!base::ReadFileToString(path, &contents))
    return base::Optional<std::string>();
  return contents;
}

bool IsSuperReferralPath(const std::string& path) {
  return path.rfind(kSuperReferralPath, 0) == 0;
}

}  // namespace

NTPBackgroundImagesSource::NTPBackgroundImagesSource(
    NTPBackgroundImagesService* service)
    : service_(service),
      weak_factory_(this) {
}

NTPBackgroundImagesSource::~NTPBackgroundImagesSource() = default;

std::string NTPBackgroundImagesSource::GetSource() {
  return kBrandedWallpaperHost;
}

void NTPBackgroundImagesSource::StartDataRequest(
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

  // Favicon data is fetched from cached folder not from component data.
  if (IsTopSiteFaviconPath(path)) {
    GetImageFile(GetTopSiteFaviconFilePath(path), std::move(callback));
    return;
  }

  auto* images_data =
      service_->GetBackgroundImagesData(IsSuperReferralPath(path));
  if (!images_data) {
    base::PostTask(FROM_HERE,
        base::BindOnce(std::move(callback),
                       scoped_refptr<base::RefCountedMemory>()));
    return;
  }

  base::FilePath image_file_path;
  if (IsLogoPath(path)) {
    image_file_path = images_data->logo_image_file;
  } else {
    DCHECK(IsWallpaperPath(path));
    image_file_path =
        images_data->backgrounds[GetWallpaperIndexFromPath(path)].image_file;
  }

  GetImageFile(image_file_path, std::move(callback));
}

void NTPBackgroundImagesSource::GetImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback callback) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPBackgroundImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(),
                     std::move(callback)));
}

void NTPBackgroundImagesSource::OnGotImageFile(
    GotDataCallback callback,
    base::Optional<std::string> input) {
  if (!input)
    return;

  scoped_refptr<base::RefCountedMemory> bytes;
  bytes = new base::RefCountedBytes(
       reinterpret_cast<const unsigned char*>(input->c_str()), input->length());
  std::move(callback).Run(std::move(bytes));
}

std::string NTPBackgroundImagesSource::GetMimeType(const std::string& path) {
  if (IsLogoPath(path) || IsTopSiteFaviconPath(path))
    return "image/png";
  return "image/jpg";
}

bool NTPBackgroundImagesSource::AllowCaching() {
  return false;
}

bool NTPBackgroundImagesSource::IsValidPath(const std::string& path) const {
  if (IsLogoPath(path))
    return true;

  if (IsWallpaperPath(path))
    return true;

  if (IsTopSiteFaviconPath(path))
    return true;

  return false;
}

bool NTPBackgroundImagesSource::IsWallpaperPath(const std::string& path) const {
  return GetWallpaperIndexFromPath(path) != -1;
}

bool NTPBackgroundImagesSource::IsLogoPath(const std::string& path) const {
  const std::string target_logo_path =
      IsSuperReferralPath(path) ? std::string(kSuperReferralPath) + kLogoPath
                                : std::string(kSponsoredImagesPath) + kLogoPath;
  return target_logo_path.compare(path) == 0;
}

int NTPBackgroundImagesSource::GetWallpaperIndexFromPath(
    const std::string& path) const {
  const bool is_super_referral_path = IsSuperReferralPath(path);
  auto* images_data = service_->GetBackgroundImagesData(is_super_referral_path);
  if (!images_data)
    return -1;

  const int wallpaper_count = images_data->backgrounds.size();
  for (int i = 0; i < wallpaper_count; ++i) {
    const std::string generated_path =
        base::StringPrintf("%s%s%d.jpg",
                           is_super_referral_path ? kSuperReferralPath
                                                  : kSponsoredImagesPath,
                           kWallpaperPathPrefix, i);
    if (path.compare(generated_path) == 0)
      return i;
  }

  return -1;
}

bool NTPBackgroundImagesSource::IsTopSiteFaviconPath(
    const std::string& path) const {
  // Top site is only used for super referral.
  if (!IsSuperReferralPath(path))
    return false;

  return !GetTopSiteFaviconFilePath(path).empty();
}

base::FilePath NTPBackgroundImagesSource::GetTopSiteFaviconFilePath(
    const std::string& path) const {
  std::vector<std::string> list = service_->GetCachedTopSitesFaviconList();
  for (const auto& favicon_file : list) {
    base::FilePath file_path = base::FilePath::FromUTF8Unsafe(favicon_file);
    if (path.compare(kSuperReferralPath + file_path.BaseName().AsUTF8Unsafe())
        == 0)
      return file_path;
  }
  return base::FilePath();
}

}  // namespace ntp_background_images
