/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/browser/ntp_sponsored_image_source.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/ref_counted_memory.h"
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
  if (!base::ReadFileToString(path, &contents)) {
    return std::optional<std::string>();
  }
  return contents;
}

bool IsSuperReferralPath(const std::string& path) {
  return path.rfind(kSuperReferralPath, 0) == 0;
}

}  // namespace

NTPSponsoredImageSource::NTPSponsoredImageSource(
    NTPBackgroundImagesService* background_images_service)
    : background_images_service_(background_images_service) {}

NTPSponsoredImageSource::~NTPSponsoredImageSource() = default;

std::string NTPSponsoredImageSource::GetSource() {
  return kBrandedWallpaperHost;
}

void NTPSponsoredImageSource::StartDataRequest(
    const GURL& url,
    const content::WebContents::Getter& /*wc_getter*/,
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

void NTPSponsoredImageSource::GetImageFile(
    const base::FilePath& image_file_path,
    GotDataCallback callback) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFileToString, image_file_path),
      base::BindOnce(&NTPSponsoredImageSource::OnGotImageFile,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void NTPSponsoredImageSource::OnGotImageFile(GotDataCallback callback,
                                             std::optional<std::string> input) {
  if (!input) {
    return;
  }

  std::move(callback).Run(
      new base::RefCountedBytes(base::as_byte_span(*input)));
}

std::string NTPSponsoredImageSource::GetMimeType(const GURL& url) {
  const std::string path = URLDataSource::URLToRequestPath(url);
  const base::FilePath file_path = base::FilePath::FromUTF8Unsafe(path);
  if (file_path.MatchesExtension(FILE_PATH_LITERAL(".jpg")) ||
      file_path.MatchesExtension(FILE_PATH_LITERAL(".jpeg"))) {
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

bool NTPSponsoredImageSource::AllowCaching() {
  return false;
}

base::FilePath NTPSponsoredImageSource::GetLocalFilePathFor(
    const std::string& path) {
  const bool is_super_referral_path = IsSuperReferralPath(path);
  const NTPSponsoredImagesData* const images_data =
      background_images_service_->GetSponsoredImagesData(
          is_super_referral_path,
          /*supports_rich_media=*/false);
  CHECK(images_data);

  const base::FilePath basename_from_path =
      base::FilePath::FromUTF8Unsafe(path).BaseName();

  for (const auto& top_site : images_data->top_sites) {
    if (top_site.image_file.BaseName() == basename_from_path) {
      return top_site.image_file;
    }
  }

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& creative : campaign.creatives) {
      const base::FilePath logo_basename_from_data =
          creative.logo.image_file.BaseName();
      const base::FilePath wallpaper_basename_from_data =
          creative.file_path.BaseName();

      if (logo_basename_from_data == basename_from_path) {
        return creative.logo.image_file;
      }

      if (wallpaper_basename_from_data == basename_from_path) {
        return creative.file_path;
      }
    }
  }

  // Should give valid path always here because invalid |path| was
  // already filtered by `IsValidPath()`.
  NOTREACHED();
}

bool NTPSponsoredImageSource::IsValidPath(const std::string& path) const {
  const bool is_super_referral_path = IsSuperReferralPath(path);
  const NTPSponsoredImagesData* const images_data =
      background_images_service_->GetSponsoredImagesData(
          is_super_referral_path,
          /*supports_rich_media=*/false);
  if (!images_data) {
    return false;
  }

  const base::FilePath basename_from_path =
      base::FilePath::FromUTF8Unsafe(path).BaseName();

  for (const auto& top_site : images_data->top_sites) {
    if (top_site.image_file.BaseName() == basename_from_path) {
      return true;
    }
  }

  for (const auto& campaign : images_data->campaigns) {
    for (const auto& creative : campaign.creatives) {
      const base::FilePath logo_basename_from_data =
          creative.logo.image_file.BaseName();
      const base::FilePath wallpaper_basename_from_data =
          creative.file_path.BaseName();

      if (logo_basename_from_data == basename_from_path ||
          wallpaper_basename_from_data == basename_from_path) {
        return true;
      }
    }
  }

  return false;
}

}  // namespace ntp_background_images
