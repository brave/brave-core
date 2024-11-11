/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_source.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "brave/components/ntp_background_images/browser/ntp_background_images_service.h"
#include "brave/components/ntp_background_images/browser/ntp_sponsored_images_data.h"
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

bool IsSuperReferralPath(const std::string& path) {
  return path.rfind(kSuperReferralPath, 0) == 0;
}

}  // namespace

NTPSponsoredImagesSource::NTPSponsoredImagesSource(
    NTPBackgroundImagesService* service)
    : service_(service), weak_factory_(this) {}

NTPSponsoredImagesSource::~NTPSponsoredImagesSource() = default;

std::string NTPSponsoredImagesSource::GetSource() {
  return kBrandedWallpaperHost;
}

void NTPSponsoredImagesSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& wc_getter,
    GotDataCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string path = URLDataSource::URLToRequestPath(url);
  if (!IsValidPath(path)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback),
                                  scoped_refptr<base::RefCountedMemory>()));

    return;
  }

  base::FilePath image_file_path = GetLocalFilePathFor(path);
  CHECK(!image_file_path.empty());
  GetImageFile(image_file_path, std::move(callback));
}

void NTPSponsoredImagesSource::GetImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPSponsoredImagesSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredImagesSource::OnGotImageFile(
    GotDataCallback callback,
    std::optional<std::string> input) {
  if (!input)
    return;

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string NTPSponsoredImagesSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  const auto file_path = base::FilePath::FromUTF8Unsafe(path);
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
      file_path.MatchesExtension(FILE_PATH_LITERAL(".jpeg"))) {
    return "image/jpeg";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".png"))) {
    return "image/png";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".webp"))) {
    return "image/webp";
  } else if (file_path.MatchesExtension(FILE_PATH_LITERAL(".avif"))) {
    return "image/avif";
  } else {
    return "";
  }
}

bool NTPSponsoredImagesSource::AllowCaching() {
  return false;
}

base::FilePath NTPSponsoredImagesSource::GetLocalFilePathFor(
    const std::string& path) {
  const bool is_super_referral_path = IsSuperReferralPath(path);
  auto* images_data = service_->GetBrandedImagesData(is_super_referral_path);
  CHECK(images_data);

  const auto basename_from_path =
      base::FilePath::FromUTF8Unsafe(path).BaseName();

  for (const auto& topsite : images_data->top_sites) {
    if (topsite.image_file.BaseName() == basename_from_path)
      return topsite.image_file;
  }

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& background : campaign.backgrounds) {
      const auto logo_basename_from_data =
          background.logo.image_file.BaseName();
      const auto wallpaper_basename_from_data =
          background.image_file.BaseName();

      if (logo_basename_from_data == basename_from_path)
        return background.logo.image_file;

      if (wallpaper_basename_from_data == basename_from_path)
        return background.image_file;
    }
  }

  // Should give valid path always here because invalid |path| was
  // already filtered by `IsValidPath()`.
  NOTREACHED();
}

bool NTPSponsoredImagesSource::IsValidPath(const std::string& path) const {
  const bool is_super_referral_path = IsSuperReferralPath(path);
  auto* images_data = service_->GetBrandedImagesData(is_super_referral_path);
  if (!images_data)
    return false;

  const auto basename_from_path =
      base::FilePath::FromUTF8Unsafe(path).BaseName();

  for (const auto& topsite : images_data->top_sites) {
    if (topsite.image_file.BaseName() == basename_from_path)
      return true;
  }

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& background : campaign.backgrounds) {
      const auto logo_basename_from_data =
          background.logo.image_file.BaseName();
      const auto wallpaper_basename_from_data =
          background.image_file.BaseName();

      if (logo_basename_from_data == basename_from_path ||
          wallpaper_basename_from_data == basename_from_path)
        return true;
    }
  }

  return false;
}

}  // namespace ntp_background_images
